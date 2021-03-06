/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#include <Visus/PythonEngine.h>
#include <Visus/Thread.h>
#include <Visus/Log.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/Path.h>
#include <Visus/File.h>

#include <cctype>

#include <pydebug.h>

#if PY_MAJOR_VERSION <3
  #define char2wchar(arg) ((char*)arg)
#else

  static wchar_t* char2wchar(const char* value) 
  {
  #if PY_MINOR_VERSION<=4
    return _Py_char2wchar((char*)value, NULL);
  #else
    return Py_DecodeLocale((char*)value, NULL);
  #endif
  }

#endif


namespace Visus {

PyThreadState* PythonEngine::mainThreadState=nullptr;

static std::set<String> ReservedWords =
{
  "and", "del","from","not","while","as","elif","global","or","with","assert", "else","if",
  "pass","yield","break","except","import","print", "class","exec""in","raise","continue", 
  "finally","is","return","def","for","lambda","try"
};


///////////////////////////////////////////////////////////////////////////
ScopedAcquireGil::ScopedAcquireGil() 
{
  state = new PyGILState_STATE();
  *state = PyGILState_Ensure();
}


///////////////////////////////////////////////////////////////////////////
ScopedAcquireGil::~ScopedAcquireGil()
{
  if (state)
  {
    PyGILState_Release(*state);
    delete state;
  }
}

///////////////////////////////////////////////////////////////////////////
ScopedReleaseGil::ScopedReleaseGil()
  : state(PyEval_SaveThread()) {
}


///////////////////////////////////////////////////////////////////////////
ScopedReleaseGil::~ScopedReleaseGil() {
  PyEval_RestoreThread(state);
}


////////////////////////////////////////////////////////////////////////////////////
bool PythonEngine::isGoodVariableName(String name)
{
  if (name.empty() || ReservedWords.count(name))
    return false;

  if (!std::isalpha(name[0]))
    return false;

  for (int I = 1; I<(int)name.length(); I++)
  {
    if (!(std::isalnum(name[I]) || name[I] == '_'))
      return false;
  }

  return true;
};

///////////////////////////////////////////////////////////////////////////
static bool runningInsidePyMain() 
{
  //no one has already callset SetCommandLine
  const auto& args = ApplicationInfo::args;
  return args.empty() || args[0].empty() || args[0] == "__main__";
}


///////////////////////////////////////////////////////////////////////////
void InitPython()
{
  if (runningInsidePyMain())
  {
    VisusInfo() << "Visus is running (i.e. extending) python";
  }
  else
  {
  	VisusInfo() << "Initializing embedded python...";
  	
	  Py_VerboseFlag = 0;
	  auto& args = ApplicationInfo::args;
	  std::vector<String> new_args;
	  for (int I = 0; I < args.size(); I++)
	  {
	    if (StringUtils::startsWith(args[I],"-v")) {
	      Py_VerboseFlag = (int)args[I].size()-1;
	      continue;
	    }
	    new_args.push_back(args[I]);
	  }
	  args = new_args;  	
  	
	  const char* arg0 = ApplicationInfo::args[0].c_str();
	  Py_SetProgramName(char2wchar(arg0));
	}  	
	
	#if 0
#if PY_MAJOR_VERSION <3
  #define CheckForNull(value) ((value)? (value) : "<null>")
	std::out
		<<" Py_GetBuildInfo()="<<CheckForNull(Py_GetBuildInfo()) << std::endl
		<<" Py_GetCompiler ()="<<CheckForNull(Py_GetCompiler ()) << std::endl
		<<" Py_GetCopyright()="<<CheckForNull(Py_GetCopyright()) << std::endl
		<<" Py_GetPlatform ()="<<CheckForNull(Py_GetPlatform ()) << std::endl
		<<" Py_GetVersion  ()="<<CheckForNull(Py_GetVersion  ()) << std::endl;		
#else
  #define CheckForNull(value) ((value)? (value) : L"<null>")
	std::wcout
		<<L" Py_GetBuildInfo()="<<CheckForNull(Py_GetBuildInfo()) << std::endl
		<<L" Py_GetCompiler ()="<<CheckForNull(Py_GetCompiler ()) << std::endl
		<<L" Py_GetCopyright()="<<CheckForNull(Py_GetCopyright()) << std::endl
		<<L" Py_GetPlatform ()="<<CheckForNull(Py_GetPlatform ()) << std::endl
		<<L" Py_GetVersion  ()="<<CheckForNull(Py_GetVersion  ()) << std::endl;		
#endif
#endif

		
	 if (!runningInsidePyMain())
	 {
	  //IMPORTANT: if you want to avoid the usual sys.path initialization
	  //you can copy the python shared library (example: python36.dll) and create a file with the same name and _pth extension
	  //(example python36_d._pth). in that you specify the directories to include. you can also for example a python36.zip file
	  //or maybe you can set PYTHONHOME
	
	  //skips initialization registration of signal handlers
	  Py_InitializeEx(0);

	  // acquire the gil
	  PyEval_InitThreads();
	
	  //NOTE if you try to have multiple interpreters (Py_NewInterpreter) I get deadlock
	  //see https://issues.apache.org/jira/browse/MODPYTHON-217
	  //see https://trac.xapian.org/ticket/185
	  PythonEngine::mainThreadState = PyEval_SaveThread();
	}
	
	#if 0
#if PY_MAJOR_VERSION <3
	std::out
		<<" Py_GetPath           ()="<<CheckForNull(Py_GetPath           ()) << std::endl	
		<<" Py_GetPrefix         ()="<<CheckForNull(Py_GetPrefix         ()) << std::endl	
		<<" Py_GetExecPrefix     ()="<<CheckForNull(Py_GetExecPrefix     ()) << std::endl	
		<<" Py_GetProgramFullPath()="<<CheckForNull(Py_GetProgramFullPath()) << std::endl	
		<<" Py_GetPythonHome     ()="<<(CheckForNull(Py_GetPythonHome    ()) << std::endl	
		<<" Py_GetProgramName    ()="<<CheckForNull(Py_GetProgramName    ()) << std::endl	;		
#else
	std::wcout
		<<L" Py_GetPath           ()="<<CheckForNull(Py_GetPath           ()) << std::endl	
		<<L" Py_GetPrefix         ()="<<CheckForNull(Py_GetPrefix         ()) << std::endl	
		<<L" Py_GetExecPrefix     ()="<<CheckForNull(Py_GetExecPrefix     ()) << std::endl	
		<<L" Py_GetProgramFullPath()="<<CheckForNull(Py_GetProgramFullPath()) << std::endl	
		<<L" Py_GetPythonHome     ()="<<(CheckForNull(Py_GetPythonHome    ()) << std::endl	
		<<L" Py_GetProgramName    ()="<<CheckForNull(Py_GetProgramName    ()) << std::endl	;		
#endif

#endif

	
  VisusInfo() << "Python initialization done";
}


///////////////////////////////////////////////////////////////////////////
void ShutdownPython()
{
  if (runningInsidePyMain())
    return;

  //VisusInfo() << "Shutting down python...";
  PyEval_RestoreThread(PythonEngine::mainThreadState);
  Py_Finalize();

  //VisusInfo() << "Python shutting down done";
}

////////////////////////////////////////////////////////////////////////////////////
void PythonEngine::incrRef(PyObject* value) {
  Py_INCREF(value);
}

////////////////////////////////////////////////////////////////////////////////////
void PythonEngine::decrRef(PyObject* value) {
  Py_DECREF(value);
}

////////////////////////////////////////////////////////////////////////////////////
String PythonEngine::fixPath(String value) {
#if WIN32
  return StringUtils::replaceAll(value, "/", "\\\\");
#else
  return StringUtils::replaceAll(value, "\\\\", "/");
#endif
}

////////////////////////////////////////////////////////////////////////////////////
void PythonEngine::addSysPath(String value,bool bVerbose) {
  value = fixPath(value);
  auto cmd = "import sys; sys.path.append('" + value + "')";
  if (bVerbose)
    VisusInfo() << cmd;
  execCode(cmd);
}

static std::atomic<int> module_id(0);

  ///////////////////////////////////////////////////////////////////////////
PythonEngine::PythonEngine(bool bVerbose) 
{
  this->module_name = StringUtils::format() << "__PythonEngine__" << (++module_id);
  VisusInfo() << "Creating PythonEngine "<< module_name <<"...";

  ScopedAcquireGil acquire_gil;
  this->module  = PyImport_AddModule(module_name.c_str()); 
  VisusReleaseAssert(this->module);

  this->globals = PyModule_GetDict(module); //borrowed

  auto builtins = PyEval_GetBuiltins(); VisusAssert(builtins);
  PyDict_SetItemString(this->globals, "__builtins__", builtins);

  //add the directory of the executable
  if (runningInsidePyMain())
  {
    if (bVerbose)
      VisusInfo() << "Visus is extending Python";
  }
  else
  {
    if (bVerbose)
      VisusInfo() << "Visus is embedding Python";

    //try to find where visus VisusiKernelPy.py files are
    {
      auto current_application_dir = KnownPaths::CurrentApplicationFile.getParent().toString();

      std::vector<String> candidates;

      //this is needed in build
      candidates.push_back(current_application_dir);

      //this is needed for install
#if __APPLE__ 
      //example: ${CMAKE_INSTALL_PREFIX}/bin/<name>.app/Contents/MacOS/<name>
      candidates.push_back(current_application_dir + "/../../../..");
#else
      //example ${CMAKE_INSTALL_PREFIX}/bin
      candidates.push_back(current_application_dir + "/..");

#endif

      for (auto dir : candidates)
      {
        auto filename=dir + "/VisusKernelPy.py";
        if (FileUtils::existsFile(filename))
        {
          if (bVerbose)
            VisusInfo() << "Found " << filename;
          addSysPath(dir, bVerbose);
          break;
        }
        else
        {
          if (bVerbose)
            VisusInfo() << "Not found " << filename;
        }
      }
    }
  }

  if (auto VISUS_PYTHON_SYS_PATH=getenv("VISUS_PYTHON_SYS_PATH"))
    addSysPath(VISUS_PYTHON_SYS_PATH,bVerbose);

	if (bVerbose)
    VisusInfo() << "Trying to import VisusKernelPy...";

  execCode("from VisusKernelPy import *");

  if (bVerbose)
    VisusInfo() << "...imported OpenVisus";

  swig_type_aborted = SWIG_TypeQuery("Visus::Aborted *");
  VisusAssert(swig_type_aborted);

  swig_type_array = SWIG_TypeQuery("Visus::Array *");
  VisusAssert(swig_type_array);

  //swig_type_object_type_info = SWIG_TypeQuery("Visus::SharedPtr< Visus::Object > *");
  //VisusAssert(object_type_info);
}


///////////////////////////////////////////////////////////////////////////
PythonEngine::~PythonEngine()
{
  VisusInfo() << "Destroying PythonEngine " << this->module_name << "...";

  ScopedAcquireGil acquire_gil;

  if (__redirect_output__) {
    execCode("__redirect_output__.restoreOldIO()");
    delModuleAttr("__redirect_output__");
    __redirect_output__ = nullptr;
  }

  // Delete the module from sys.modules
  PyObject* modules = PyImport_GetModuleDict();
  PyDict_DelItemString(modules, module_name.c_str());
}


///////////////////////////////////////////////////////////////////////////
int PythonEngine::main(std::vector<String> args)
{
#if PY_MAJOR_VERSION>=3
  typedef wchar_t* ArgType;
  #define PyNewArg(arg)  char2wchar(arg.c_str())
  #define PyFreeArg(arg) PyMem_RawFree(arg)
#else
  typedef char* ArgType;
  #define PyNewArg(arg) ((char*)arg.c_str())
  #define PyFreeArg(arg)
#endif

  static int     py_argn = 0;
  static ArgType py_argv[1024];
  for (auto arg : args)
    py_argv[py_argn++] = PyNewArg(arg);

  Py_SetProgramName(py_argv[0]);
  //Py_Initialize();
  int ret = Py_Main(py_argn, py_argv);
  Py_Finalize();

  for (int I = 0; I < py_argn; I++)
    PyFreeArg(py_argv[I]);

  #undef NewPyArg
  #undef FreePyArg

  return ret;
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::setModuleAttr(String name, PyObject* value) {
  PyDict_SetItemString(globals, name.c_str(), value);
}


///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::getModuleAttr(String name) {
  return PyDict_GetItemString(globals, name.c_str()); //borrowed
}

///////////////////////////////////////////////////////////////////////////
bool PythonEngine::hasModuleAttr(String name) {
  return getModuleAttr(name) ? true : false;
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::delModuleAttr(String name) {
  if (hasModuleAttr(name))
    PyDict_DelItemString(globals, name.c_str());
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::setError(String explanation, PyObject* err)
{
  PyErr_SetString(err, explanation.c_str());
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::internalNewPyFunction(PyObject* self, String name, Function fn)
{
  //see http://code.activestate.com/recipes/54352-defining-python-class-methods-in-c/
  //see http://bannalia.blogspot.it/2016/07/passing-capturing-c-lambda-functions-as.html
  //see https://stackoverflow.com/questions/26716711/documentation-for-pycfunction-new-pycfunction-newex

  class PyCapsuleInfo
  {
  public:
    UniquePtr<PyMethodDef> mdef;
    Function               fn;
    PyObject*              self = nullptr;
  };

  //callFunction
  auto callFunction = [](PyObject* py_capsule, PyObject* args)->PyObject*
  {
    auto info = static_cast<PyCapsuleInfo*>(PyCapsule_GetPointer(py_capsule, nullptr));
    return info->fn(info->self, args);
  };

  auto info = new PyCapsuleInfo();
  info->mdef.reset(new PyMethodDef({ name.c_str(), callFunction, METH_VARARGS,nullptr }));
  info->fn = fn;
  info->self = self;

  auto py_capsule = PyCapsule_New(info, nullptr, /*destrutor*/[](PyObject* py_capsule) {
    auto info = static_cast<PyCapsuleInfo*>(PyCapsule_GetPointer(py_capsule, nullptr));
    delete info;
  });

  auto ret = PyCFunction_NewEx(/*method definition*/info->mdef.get(), /*self*/py_capsule,/*module*/self ? nullptr : this->module);
  Py_DECREF(py_capsule);
  return ret;
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::addModuleFunction(String name, Function fn)
{
  auto py_fn = internalNewPyFunction(/*self*/nullptr, name, fn);
  setModuleAttr(name, py_fn);
  Py_DECREF(py_fn);
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::addObjectMethod(PyObject* self, String name, Function fn)
{
  auto py_fn = internalNewPyFunction(self, name, fn);
  auto py_name = PyString_FromString(name.c_str());
  PyObject_SetAttr(self, py_name, py_fn);
  Py_DECREF(py_fn);
  Py_XDECREF(py_name);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(double value) {
  return PyFloat_FromDouble(value);;
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(int value) {
  return PyLong_FromLong(value);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(String s) {
  return PyString_FromString(s.c_str());
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(Aborted value) {
  return newPyObject<Aborted>(value, swig_type_aborted);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(Array value) {
  return newPyObject<Array>(value, swig_type_array);
}

///////////////////////////////////////////////////////////////////////////
Aborted PythonEngine::getModuleAbortedAttr(String name) {
  return getSwigModuleAttr<Aborted>(name, swig_type_aborted);
}

///////////////////////////////////////////////////////////////////////////
Array PythonEngine::getModuleArrayAttr(String name) {
  return getSwigModuleAttr<Array>(name, swig_type_array);
}

///////////////////////////////////////////////////////////////////////////
Array PythonEngine::toArray(PyObject* py_object) {

  Array* ptr = nullptr;
  int res = SWIG_ConvertPtr(py_object, (void**)&ptr, swig_type_array, 0);

  if (!SWIG_IsOK(res) || !ptr)
    ThrowException(StringUtils::format() << "cannot convert to array");

  Array ret = (*ptr);

  if (SWIG_IsNewObj(res))
    delete ptr;

  return ret;
}


///////////////////////////////////////////////////////////////////////////
void PythonEngine::redirectOutputTo(std::function<void(String)> value)
{
  ScopedAcquireGil acquire_gil;

  execCode(
    "import sys\n"
    "class RedirectOutput :\n"
    "  def __init__(self) :\n"
    "    self.stdout,self.stderr = (sys.stdout,sys.stderr)\n"
    "    sys.stdout,sys.stderr   = (self,self)\n"
    "  def __del__(self) :\n"
    "    self.restoreOldIO()\n"
    "  def restoreOldIO(self): \n"
    "    sys.stdout,sys.stderr = (self.stdout,self.stderr)\n"
    "  def write(self, msg) :\n"
    "   self.internalWrite(msg)\n"
    "  def flush(self) :\n"
    "    pass\n"
    "__redirect_output__=RedirectOutput()\n"
  );

  __redirect_output__ = getModuleAttr("__redirect_output__");
  VisusAssert(__redirect_output__);

  addObjectMethod(__redirect_output__, "internalWrite", [value](PyObject*, PyObject* args)
  {
    VisusAssert(PyTuple_Check(args));
    for (int I = 0, N = (int)PyTuple_Size(args); I < N; I++) {
      auto obj = PyTuple_GetItem(args, I);
      auto s = PythonEngine::convertToString(obj);
      value(s);
    }
    auto ret = Py_None;
    incrRef(ret);
    return ret;
  });

}

///////////////////////////////////////////////////////////////////////////
String PythonEngine::convertToString(PyObject* value)
{
  if (!value) 
    return "";
  
  PyObject *py_str = PyObject_Str(value);
  auto tmp = SWIG_Python_str_AsChar(py_str);
  String ret = tmp ? tmp : "";
  SWIG_Python_str_DelForPy3(tmp);
  Py_DECREF(py_str);
  return ret;
}

///////////////////////////////////////////////////////////////////////////
String PythonEngine::getLastErrorMessage()
{
  //see http://www.solutionscan.org/154789-python
#if 1
  auto err = PyErr_Occurred();
  if (!err)
    return "";

  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);

  std::ostringstream out;

  out << "Python error: " 
    << convertToString(type) << " "
    << convertToString(value) << " ";

  auto module_name = PyString_FromString("traceback");
  auto module = PyImport_Import(module_name);
  Py_DECREF(module_name);

  auto fn = module? PyObject_GetAttrString(module, "format_exception") : nullptr;
  if (fn && PyCallable_Check(fn))
  {
    if (auto descr = PyObject_CallFunctionObjArgs(fn, type, value, traceback, NULL))
    {
      out << convertToString(descr);
      Py_DECREF(descr);
    }
  }

  auto ret = out.str();
  return ret;

#else
  PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  PyErr_NormalizeException(&type, &value, &traceback);

  PyErr_Clear();

  auto stype = convertToString(type);
  auto svalue = convertToString(value);
  auto straceback = convertToString(traceback);

  std::ostringstream out;
  out<<"Python error: "<< stype <<" "<< svalue  << " "<< straceback;
  return out.str();
#endif
}

///////////////////////////////////////////////////////////////////////////
static void PythonPrintCrLfIfNeeded()
{
#if PY_MAJOR_VERSION < 3

  //this returns !=1 in case of errors
  if (Py_FlushLine()) 
    PyErr_Clear();

#endif

}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::execCode(String s)
{
  auto obj = PyRun_StringFlags(s.c_str(), Py_file_input, globals, globals, nullptr);
  bool bError = (obj == nullptr);

  if (bError)
  {
    if (PyErr_Occurred())
    {
      auto  error_msg = getLastErrorMessage();
      PyErr_Clear();
      ThrowException(error_msg);
    }
  }

  Py_DECREF(obj);
  PythonPrintCrLfIfNeeded();
};




///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::evalCode(String s)
{
  //see https://bugs.python.org/issue405837
  //Return value: New reference.
  auto obj = PyRun_StringFlags(s.c_str(), Py_eval_input, globals, globals, nullptr);

  if (bool bMaybeError = (obj == nullptr))
  {
    if (PyErr_Occurred())
    {
      auto  error_msg = getLastErrorMessage();
      PyErr_Clear();
      ThrowException(error_msg);
    }
  }

  PythonPrintCrLfIfNeeded();
  return obj;
};


} //namespace Visus

