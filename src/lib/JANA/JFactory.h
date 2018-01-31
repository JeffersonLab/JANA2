//
//    File: JFactory.h
// Created: Fri Oct 20 09:44:48 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifndef _JFactory_h_
#define _JFactory_h_

#include <vector>

#include "JFactoryBase.h"

template <typename DataType>
class JFactory : public JFactoryBase
{
	public:

		using IteratorType = typename std::vector<DataType>::const_iterator;
		using PairType = std::pair<IteratorType, IteratorType>;

		JFactory(std::string aName, std::string aTag = "");
		virtual ~JFactory() = 0;

		void Set(std::vector<DataType>&& aData);
		PairType Get(void) const;
		std::type_index GetObjectType(void) const;
		
		void ClearData(void);

	protected:

		std::vector<DataType> mData;
};

//---------------------------------
// JFactory
//---------------------------------
template <typename DataType>
inline JFactory<DataType>::JFactory(std::string aName, std::string aTag) : JFactoryBase(aName, aTag)
{
}

//---------------------------------
// ~JFactory
//---------------------------------
template <typename DataType>
inline JFactory<DataType>::~JFactory(void)
{
}

//---------------------------------
// GetObjectType
//---------------------------------
template <typename DataType>
inline std::type_index JFactory<DataType>::GetObjectType(void) const
{
	return std::type_index(typeid(DataType));
}

//---------------------------------
// Set
//---------------------------------
template <typename DataType>
inline void JFactory<DataType>::Set(std::vector<DataType>&& aData)
{
	mData = std::move(aData);
}

//---------------------------------
// Get
//---------------------------------
template <typename DataType>
inline typename JFactory<DataType>::PairType JFactory<DataType>::Get(void) const
{
	return std::make_pair(mData.cbegin(), mData.cend());
}

//---------------------------------
// ClearData
//---------------------------------
template <typename DataType>
inline void JFactory<DataType>::ClearData(void)
{
	mData.clear();
}

#endif // _JFactory_h_

