
import sys, os, sip

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
import numpy as np

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
def save(data,dtype,i):
  # cast it to Visus::Array
  buffer=data.get().toArray()
  # convert Visus:Array to numpy array
  np_array=buffer.toNumPy()
  filename='heaf_'+str(np_array.shape[2])+"_"+str(np_array.shape[1])+"_"+str(np_array.shape[0])+"_"+i+".raw"
  # save numpy array to a .raw file
  np_array.astype(dtype).tofile('/Users/venkat1/research/datasets/'+filename)

# ///////////////////////////////////////////////////////////
# def setCamera(viewer):
#   # finding query node
#   camera_node=viewer.findNodeByName("glcamera")
#   stree=StringTree("GLCameraNode","decoded_typename","GLCameraNode","name","glcamera")
#   glcamera=StringTree("glcamera","TypeName","GLLookAtCamera")
#   pos=StringTree("pos","value","-100 -17 7000")
#   bound=StringTree("bound","value","0 0 0 1259 1253 7781")
#   vup=StringTree("vup","value","0 1 0")
#   direction=StringTree("dir","value","1 1 1")
#   centerOfRotation=StringTree("centerOfRotation","value","335 335 6225")
#   defaultRotFactor=StringTree("defaultRotFactor","value","5.2")
#   defaultPanFactor=StringTree("defaultPanFactor","value","30")
#   disableRotation=StringTree("disableRotation","value","False")
#   bUseOrthoProjection=StringTree("bUseOrthoProjection","value","False")
#   bAutoOrthoParams=StringTree("bAutoOrthoParams","value","True")
#   glcamera.addChild(pos)
#   glcamera.addChild(bound)
#   glcamera.addChild(vup)
#   glcamera.addChild(direction)
#   glcamera.addChild(centerOfRotation)
#   glcamera.addChild(defaultRotFactor)
#   glcamera.addChild(defaultPanFactor)
#   glcamera.addChild(disableRotation)
#   glcamera.addChild(bUseOrthoProjection)
#   glcamera.addChild(bAutoOrthoParams)
#   stree.addChild(glcamera)
#   PrintDebugMessage(stree.toString())
#   stream=ObjectStream(stree,ord('r'))
#   camera_node.readFromObjectStream(stream)

# ///////////////////////////////////////////////////////////
def modifyQuery(viewer,zstart,zend):
  # finding query node
  query_node=viewer.findNodeByName("Volume 1")
  stree=StringTree("QueryNode","decoded_typename","QueryNode","name","Volume 1")
  accessindex=StringTree("accessindex","value","0")
  view_dependent=StringTree("view_dependent","value","0")
  progression=StringTree("progression","value","0")
  quality=StringTree("quality","value","0")
  bounds=StringTree("bounds","pdim","3")
  box=StringTree("box")
  box.addChild(StringTree("p1","value","240 240 "+zstart))
  box.addChild(StringTree("p2","value","1010 1010 "+zend))
  bounds.addChild(box)
  stree.addChild(accessindex)
  stree.addChild(view_dependent)
  stree.addChild(progression)
  stree.addChild(quality)
  stree.addChild(bounds)
  stream=ObjectStream(stree,ord('r'))
  query_node.readFromObjectStream(stream)

# ///////////////////////////////////////////////////////////
def addScript(viewer):
  scripting_node=viewer.findNodeByName("Scripting")
  # adding scripting code
  stree=StringTree("ScriptingNode","decoded_typename","ScriptingNode","name","Scripting")
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
    " k=1/(np.sqrt(np.pi)*sigma)\r" \
    " total_weight=0.0\r" \
    " z=[]\r" \
    " for i in range(left,left+size):\r" \
    "   x=np.linspace(i-0.5,i+0.5,samples_per_bin)\r" \
    "   y=k*np.exp(-((x**2)*(1/(2*sigma*sigma))))\r" \
    "   z.append(simps(y,x))\r" \
    "   total_weight+=z[-1]\r"\
    " return z/total_weight\r\r" \
    "g=gaussiankernel(11,8.0)\r" \
    "g1=(np.matrix(g)).round(decimals=6)\r" \
    "g2=np.transpose(g1)\r" \
    "g3=g2*g1\r" \
    "a=np.ceil(g3/g3[0,0])\r" \
    "kernel=a/a.sum()\r\r"\
    "blur=ArrayUtils.convolve(input,Array.fromNumPy(kernel),aborted)\r" \
    "temp=blur.toNumPy()\r" \
    "temp[temp<0.275]=1.0\r" \
    "temp[temp>0.35]=1.0\r" \
    "output=Array.fromNumPy(temp)"
  stree.addChild(StringTree("code")).addChild(StringTree("#text","value",script))
  stream=ObjectStream(stree,ord('r'))
  scripting_node.readFromObjectStream(stream)

# ///////////////////////////////////////////////////////////
def addPalette(viewer):
# set palette
  palette_node=viewer.findNodeByName("Palette")
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
  palette_node.beginUpdate()
  palette_node.readFromObjectStream(stream)
  palette_node.endUpdate()

# ///////////////////////////////////////////////////////////
def main(zstart,zend,i):
  viewer=Viewer()
  viewer.openFile("file:///Users/venkat1/research/datasets/grain_boundaries_1a.idx")
  dataset_node=toDataset(viewer.findNodeByName("file:///Users/venkat1/research/datasets/grain_boundaries_1a.idx"))
  dataset_node.setShowBounds(False)
  
  root=viewer.getRoot()
  pynode=ExtractSubset()
  viewer.addNode(root,pynode)
  
  #setCamera(viewer)
  modifyQuery(viewer,zstart,zend)  
  addScript(viewer)
  addPalette(viewer)
  
  # pynode will get the data from the scripting
  viewer.connectPorts(viewer.findNodeByName("Scripting"),"data","data",pynode)  
  GuiModule.execApplication()
  # get data from pynode and save as raw
  data=pynode.readInput("data")
  dtype=dataset_node.getDataset().get().getDefaultField().dtype.toString()
  save(data,dtype,i)
  
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
  
  zstack=["6150","6300","6450","6600","6700","6850"]
  for i in range(0,len(zstack),2):
    main(zstack[i], zstack[i+1],str(int(i/2)))
  forceGC()
  
  AppKitModule.detach()
  sys.exit(0)
