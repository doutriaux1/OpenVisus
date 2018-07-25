
import sys, os, sip, stat

from OpenVisus import *

import PyQt5
from PyQt5.QtCore    import *
from PyQt5.QtWidgets import *
from PyQt5.QtGui     import *

from VisusNodesPy   import *
from VisusKernelPy   import *
from VisusDataflowPy import *
from VisusGuiPy      import *
from VisusGuiNodesPy import *
from VisusAppKitPy   import *
import argparse
from subprocess import call, check_call

def convertToQWidget(value):
  return VisusGuiPy.convertToQWidget(sip.unwrapinstance(value))

# ///////////////////////////////////////////////////////////
class MyWidget(QWidget):
  def onKernelChanged(self):
    if int(self.kernel[0].text())%2 == 0:
      QMessageBox.information("","Kernel size should be odd")
      return
    modifyScript(self.kernel[0].text())

  def onQueryChanged(self,value):
    start=self.query_start[0].text()+" "+self.query_start[1].text()+" "+self.query_start[2].text()
    end=self.query_end[0].text()+" "+self.query_end[1].text()+" "+self.query_end[2].text()
    modifyQuery(start,end)

  # __init__
  def __init__(self,start,end):
    QWidget.__init__(self)
    layout=QFormLayout()
    rows=[QHBoxLayout(),QHBoxLayout(),QHBoxLayout(),QHBoxLayout(),QHBoxLayout()]
    s=start.split(" ")
    e=end.split(" ")
    self.query_start=[QLineEdit(s[0]),QLineEdit(s[1]),QLineEdit(s[2])]
    self.query_end=[QLineEdit(e[0]),QLineEdit(e[1]),QLineEdit(e[2])]
    self.kernel=[QLineEdit("8.0")]
    for (item1,item2) in zip(self.query_start,self.query_end):
      rows[0].addWidget(item1)
      rows[1].addWidget(item2)
    self.querySet=QToolButton()
    self.querySet.setText("Set")
    self.querySet.clicked.connect(self.onQueryChanged)
    rows[2].addWidget(self.querySet)
    rows[3].addWidget(self.kernel[0])
    layout.addRow("Query Start",rows[0])
    layout.addRow("Query End",rows[1])
    layout.addRow("",rows[2])
    layout.addRow("Kernel Radius",rows[3])
    self.kernelSet=QToolButton()
    self.kernelSet.setText("Set")
    self.kernelSet.clicked.connect(self.onKernelChanged)
    rows[4].addWidget(self.kernelSet)
    layout.addRow("",rows[4])
    self.setLayout(layout)
    self.show()

# ///////////////////////////////////////////////////////////
class ExtractSubset(PythonNode):
  # __init__
  def __init__(self):
    PythonNode.__init__(self)
    self.setName("ExtractSubset")
    self.addInputPort("data")
    
  # getOsDependentTypeName
  def getOsDependentTypeName(self):
    return "ExtractSubset"

  # processInput
  def processInput(self):
    return PythonNode.processInput(self)

# ///////////////////////////////////////////////////////////
def Postprocess(dims,path,filename,persistence):
  msc.write("echo *****************************Computing lstar for segment "+i + "*******************************************************************************************\n\n")
  msc.write("cd $1" + "\n")
  msc.write("./Steepest_lstar "+dims[0]+" "+dims[1]+" "+dims[2]+" "+path+filename+" 0 0 0\n\n")
  msc.write("echo *****************************Computing MSC for segment "+i + "*********************************************************************************************\n")
  msc.write("cd $2" + "\n")
  msc.write("./MSPallettSeg "+path+filename+" "+dims[0]+" "+dims[1]+" "+dims[2]+" "+persistence+" 0\n\n")
  msc.write("awk '$5 == 0 && $2 < 0.5 { print $1 }' "+path+"heaf_*.raw.pers >> "+path+"temp.txt \n")
  msc.write("x=$(awk 'END{print NR}' "+path+"temp.txt) \n")
  msc.write("awk -v awk_x=$x '{ print awk_x-NR,$1 }' "+path+"temp.txt > "+path+"filtered.txt \n")
  msc.write("rm -fr "+path+"temp.txt \n\n")

# ///////////////////////////////////////////////////////////
def save(data,dtype,zstart,zend,i):
  # cast it to Visus::Array
  buffer=Array.castFrom(data.get())
  # convert Visus:Array to numpy array
  np_array=buffer.toNumPy()
  dims=[str(np_array.shape[2]),str(np_array.shape[1]),str(np_array.shape[0])]
  filename=name+'_'+dims[0]+"_"+dims[1]+"_"+dims[2]+"_"+zstart+"_"+zend+".raw"
  path=os.path.join(basepath,i+"/")
  os.makedirs(path,exist_ok=True)
    # save numpy array to a .raw file
  np_array.astype(dtype).tofile(path+filename)
  return(dims,path,filename)

# ///////////////////////////////////////////////////////////
def modifyQuery(query_start,query_end):
  # modify query node
  query_node=QueryNode.castFrom(viewer.findNodeByName("Volume 1"))
  box=Box3d(Point3d(query_start),Point3d(query_end))
  viewer.getGLCamera().get().guessPosition(box,2)
  query_node.setNodeBounds(Position(box))
  viewer.refreshData(query_node)

# ///////////////////////////////////////////////////////////
def modifyScript(kernelRadius="8.0"):
  # adding scripting code
  scripting_node=ScriptingNode.castFrom(viewer.findNodeByName("Scripting"))
  kernelSize=str(2*int(3*float(kernelRadius)+0.5)+1)

  script="import numpy as np\r\r" \
    "def simps(y,x):\r" \
    " if((x.size-1)%2):\r" \
    "   raise ValueError(\"num_of_samples must be even (received size=%d)\", x.size-1)\r" \
    " z=y[0]+y[-1]\r" \
    " for i in range(1, x.size, 2):\r" \
    "   z+=4.0*y[i]\r" \
    " for i in range(2, x.size-1, 2):\r" \
    "   z+=2.0*y[i]\r\r" \
    " h=(x[-1]-x[0])/(x.size-1)\r" \
    " return (z*h)/3.0\r" \
    "\r"\
    "def gaussiankernel(size,sigma,num_of_samples=10000):\r" \
    " left=-int(np.floor(size/2.0))\r" \
    " samples_per_bin=np.ceil(num_of_samples/size)\r" \
    " if(samples_per_bin%2 == 0):\r" \
    "   samples_per_bin+=1\r\r" \
    " total_weight=0.0\r" \
    " z=[]\r" \
    " for i in range(left,left+size):\r" \
    "   x=np.linspace(i-0.5,i+0.5,samples_per_bin)\r" \
    "   y=np.exp(-((x**2)*(1/(2*sigma*sigma))))\r" \
    "   z.append(simps(y,x))\r" \
    "   total_weight+=z[-1]\r"\
    " return z/total_weight\r\r" \
    "g=gaussiankernel("+kernelSize+","+kernelRadius+")\r" \
    "g=(np.matrix(g)).round(decimals=6)\r" \
    "g=np.ceil(g/g[0,0])\r" \
    "g=g/g.sum()\r" \
    "kernel=g.reshape(1,1,"+kernelSize+")\r" \
    "input=ArrayUtils.convolve(input,Array.fromNumPy(kernel),aborted)\r" \
    "kernel=g.reshape(1,"+kernelSize+",1)\r" \
    "input=ArrayUtils.convolve(input,Array.fromNumPy(kernel),aborted)\r" \
    "kernel=g.reshape("+kernelSize+",1,1)\r" \
    "input=ArrayUtils.convolve(input,Array.fromNumPy(kernel),aborted)\r" \
    "input=input.toNumPy()\r" \
    "input[input<0.275]=1.0\r" \
    "input[input>0.35]=1.0\r" \
    "output=Array.fromNumPy(input)"
  scripting_node.setCode(script)
  viewer.refreshData(scripting_node)

# ///////////////////////////////////////////////////////////
def modifyPalette():
# set palette
  node=viewer.findNodeByName("Palette")
  stree=StringTree("PaletteNode","decoded_typename","PaletteNode","name","Palette")
  palette=StringTree("palette","attenuation","0.0","name","GrayTransparent")
  custom_range=StringTree("custom_range")
  custom_range.addChild(StringTree("from","value","0.275"))
  custom_range.addChild(StringTree("to", "value", "0.35"))
  custom_range.addChild(StringTree("step", "value", "1.0"))
  inp=StringTree("input","mode","3")
  inp.addChild(custom_range)
  palette.addChild(inp)
  stree.addChild(palette)
  stream=ObjectStream(stree,ord('r'))
  node.readFromObjectStream(stream)
  viewer.refreshData(node)

# ///////////////////////////////////////////////////////////
def main(zstart,zend,i):
  global viewer
  viewer=Viewer()
  viewer.openFile("file://"+dataset)
  
  query_start="240 240 "+zstart
  query_end="1010 1010 "+zend

  #add custom widget
  mywidget=MyWidget(query_start,query_end)
  viewer.addDockWidget("",convertToQWidget(mywidget))
  
  #set query properties
  query_node=QueryNode.castFrom(viewer.findNodeByName("Volume 1"))
  query_node.setViewDependentEnabled(False)
  query_node.setQuality(0)
  query_node.setProgression(0)

  #disable bounds
  dataset_node=DatasetNode.castFrom(viewer.findNodeByName("file://"+dataset))
  dataset_node.setShowBounds(False)
  
  #add pynode
  root=viewer.getRoot()
  pynode=ExtractSubset()
  viewer.addNode(root,pynode)
  
  #modify query,script and palette
  modifyQuery(query_start,query_end)
  modifyScript()
  modifyPalette()
  
  # pynode will get the data from scripting
  viewer.connectPorts(viewer.findNodeByName("Scripting"),"data","data",pynode)  
  GuiModule.execApplication()
  # get data from pynode and save as raw
  data=pynode.readInput("data")
  dtype=dataset_node.getDataset().get().getDefaultField().dtype.toString()
  [dims,path,filename]=save(data,dtype,zstart,zend,i)
  Postprocess(dims,path,filename,persistence)
  
  viewer=None

# ///////////////////////////////////////////////////////////
def forceGC():
  # try to destroy the viewer here...
  import gc
  gc.collect()
  gc.collect()
  gc.collect() 
  
# ///////////////////////////////////////////////////////////
if __name__ == '__main__':

  SetCommandLine("__main__")
  GuiModule.createApplication()
  AppKitModule.attach()  
  VISUS_REGISTER_PYTHON_OBJECT_CLASS("ExtractSubset")
  
  parser=argparse.ArgumentParser(description="Extract subsets for HEAF Segmenatation")
  parser.add_argument("-d", "--dataset",help="full path to dataset")
  parser.add_argument("-o", "--out",help="path to output subset of the dataset")
  parser.add_argument("-n", "--name",help="output filename")
  parser.add_argument("-p", "--persistence",default="0.2",help="persistence for segmenatation")
  parser.add_argument("-z", "--stack",nargs="*",type=str,help="list of zstart and end of subsets to segment") 
  parser.add_argument("-lp", "--lstar_path",help="path to lstar")
  parser.add_argument("-sp", "--msc_path",help="path to msc") 
  args=parser.parse_args()
  global dataset
  dataset=args.dataset
  global basepath
  basepath=args.out
  global name
  name=args.name
  global persistence
  persistence=args.persistence


  #create script to start msc segmentation
  global msc
  os.makedirs(basepath,exist_ok=True)
  msc_filename=basepath+"start-msc.sh"
  msc=open(msc_filename, 'w')
  msc.write("#!/bin/bash" + "\n")
  msc.write("echo *****************************Starting MSC Segmentation" + "***********************************************************************************************\n\n")

  for i in range(0,len(args.stack),2):
    main(args.stack[i], args.stack[i+1],str(int(i/2)))
  forceGC()
  msc.write("exit 0" + "\n")
  msc.close()

  st=os.stat(msc_filename)
  os.chmod(msc_filename, st.st_mode | stat.S_IEXEC)
  if os.path.exists(msc_filename):
    check_call("sh "+msc_filename+" '%s' '%s'" % (args.lstar_path,args.msc_path),shell=True)
  AppKitModule.detach()
  sys.exit(0)
