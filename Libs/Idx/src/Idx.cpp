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

#include <Visus/Idx.h>
#include <Visus/IdxDataset.h>  
#include <Visus/IdxMultipleDataset.h>
#include <Visus/PythonEngine.h>

namespace Visus {

bool IdxModule::bAttached = false;

//////////////////////////////////////////////////////////////////
void IdxModule::attach()
{
  if (bAttached)  
    return;
  
  VisusInfo() << "Attaching IdxModule...";

  bAttached = true;

  DbModule::attach();

  //VISUS_REGISTER_OBJECT_CLASS(IdxFile);
  VISUS_REGISTER_OBJECT_CLASS(IdxDataset);
  VISUS_REGISTER_OBJECT_CLASS(IdxMultipleDataset);
  
  //VISUS_REGISTER_OBJECT_CLASS(IdxDiskAccess); ABSTRACT
  //VISUS_REGISTER_OBJECT_CLASS(IdxDiskAccessV5); do I need this in ObjectFactory?
  //VISUS_REGISTER_OBJECT_CLASS(IdxDiskAccessV6); do I need this in ObjectFactory?

  DatasetPluginFactory::getSingleton()->registerDatasetType(".idx" ,"IdxDataset");
  DatasetPluginFactory::getSingleton()->registerDatasetType(".midx","IdxMultipleDataset");

  IdxDataset::tryRemoveLockAndCorruptedBinaryFiles();

  VisusInfo() << "Attached IdxModule";
}


//////////////////////////////////////////////
void IdxModule::detach()
{
  if (!bAttached)  
    return;
  
  VisusInfo() << "Detatching IdxModule...";
  
  bAttached = false;

  DbModule::detach();

  VisusInfo() << "Detatched IdxModule";
}

} //namespace Visus

