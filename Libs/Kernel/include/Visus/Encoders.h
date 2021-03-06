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

#ifndef VISUS_ENCODERS_H
#define VISUS_ENCODERS_H

#include <Visus/Kernel.h>
#include <Visus/Array.h>

namespace Visus {

  //////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Encoder
{
public:

  VISUS_CLASS(Encoder)
  
  //constructor
  Encoder()
  {}
  
  //destructor
  virtual ~Encoder()
  {}

  //isLossy
  virtual bool isLossy() const=0;

  //encode
  virtual SharedPtr<HeapMemory> encode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> decoded)=0;

  //decode
  virtual SharedPtr<HeapMemory> decode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> encoded)=0;

};


//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API IdEncoder : public Encoder
{
public:

  VISUS_CLASS(IdEncoder)

  //constructor
  IdEncoder()
  {}

  //destructor
  virtual ~IdEncoder()
  {}

  //isLossy
  virtual bool isLossy() const override
  {return false;}

  //encode
  virtual SharedPtr<HeapMemory> encode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> decoded) override;

  //decode
  virtual SharedPtr<HeapMemory> decode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> encoded) override;

};

//////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API FreeImageEncoder : public Encoder
{
public:

  VISUS_CLASS(FreeImageEncoder)

  //constructor
  FreeImageEncoder(String encoder_name_);

  //destructor
  virtual ~FreeImageEncoder();

  //isLossy
  virtual bool isLossy() const override;

  //encode
  virtual SharedPtr<HeapMemory> encode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> decoded) override;

  //decode
  virtual SharedPtr<HeapMemory> decode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> encoded) override;

private:

  String encoder_name;
  int encode_flags;
  int decode_flags;

  //canEncode
  static  bool canEncode(String encoder_name,DType dtype);


};

//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ZipEncoder : public Encoder
{
public:

  VISUS_CLASS(ZipEncoder)

  //constructor
  ZipEncoder()
  {}

  //destructor
  virtual ~ZipEncoder()
  {}

  //isLossy
  virtual bool isLossy() const override
  {return false;}

  //encode
  virtual SharedPtr<HeapMemory> encode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> decoded) override;

  //decode
  virtual SharedPtr<HeapMemory> decode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> encoded) override;

};


//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API LZ4Encoder : public Encoder
{
public:

  VISUS_CLASS(LZ4Encoder)

  //constructor
  LZ4Encoder() {
  }

  //destructor
  virtual ~LZ4Encoder() {
  }

  //isLossy
  virtual bool isLossy() const override {
    return false;
  }

  //encode
  virtual SharedPtr<HeapMemory> encode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> decoded) override;

  //decode
  virtual SharedPtr<HeapMemory> decode(NdPoint dims,DType dtype, SharedPtr<HeapMemory> encoded) override;

};


//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Encoders 
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(Encoders)

  //addEncoder
  void addEncoder(String name,SharedPtr<Encoder> value)
  {
    VisusAssert(getEncoder(name)==nullptr);
    encoders[name]=value;
  }

  //getEncoder
  Encoder* getEncoder(String name)
  {
    name=StringUtils::trim(StringUtils::toLower(name));
    auto it=encoders.find(name);
    return it!=encoders.end()? it->second.get() : nullptr;
  }


private:

  std::map<String, SharedPtr<Encoder> > encoders;

  //constructor
  Encoders()
  {
    addEncoder(""   ,std::make_shared<IdEncoder>());
    addEncoder("raw", std::make_shared<IdEncoder>());
    addEncoder("bin", std::make_shared<IdEncoder>());
    addEncoder("lz4", std::make_shared<LZ4Encoder>());
    addEncoder("zip", std::make_shared<ZipEncoder>());
    addEncoder("png", std::make_shared<FreeImageEncoder>("png"));
    addEncoder("jpg", std::make_shared<FreeImageEncoder>("jpg"));
    addEncoder("tif", std::make_shared<FreeImageEncoder>("tif"));
  }


};


} //namespace Visus

#endif //VISUS_ENCODERS_H


