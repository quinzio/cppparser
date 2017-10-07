/*
   The MIT License (MIT)

   Copyright (c) 2014

   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
   the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cppprog.h"
#include "cppparser.h"

#include <boost/filesystem.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace bfs = boost::filesystem;

//////////////////////////////////////////////////////////////////////////

void CppProgram::loadProgram(const char* szInputPath)
{
	loadCppDom(szInputPath);
	// Create the type-tree for all Compound objects and type objects.
	for(CppCompoundArray::const_iterator domItr = fileDoms_.begin(); domItr != fileDoms_.end(); ++domItr)
		loadType(*domItr, &cppTypeTreeRoot_);
}

void CppProgram::addCppDom(CppCompound* cppDom)
{
	if(cppDom->compoundType_ != kCppFile)
		return;
	loadType(cppDom, &cppTypeTreeRoot_);
	fileDoms_.push_back(cppDom);
}

void CppProgram::loadCppDom(const char* szInputPath)
{
	bfs::path inputPath = szInputPath;
	if(bfs::is_regular_file(inputPath))
	{
		CppCompound* cppdom = parseSingleFile(inputPath.string().c_str());
		if(cppdom)
			fileDoms_.push_back(cppdom);
	}
	else if(bfs::is_directory(inputPath))
	{

		for(bfs::directory_iterator dirItr(inputPath); dirItr != bfs::directory_iterator(); ++dirItr)
		{
			bfs::path p = *dirItr;
			loadCppDom(p.string().c_str());
		}
	}
}

void CppProgram::loadType(CppCompound* cppCompound, CppTypeTreeNode* typeNode)
{
	if(cppCompound == NULL)
		return;
	if(cppCompound->isCppFile()) // Type node for file object should be the root itself.
	{
		cppObjToTypeNode_[cppCompound] = typeNode;
		typeNode->cppObjSet.insert(cppCompound);
	}
	for(CppObjArray::const_iterator itr = cppCompound->members_.begin(); itr != cppCompound->members_.end(); ++itr)
	{
		CppObj* mem = *itr;
		if(mem->objType_ == CppObj::kCompound)
		{
			CppTypeTreeNode& childNode = typeNode->children[((CppCompound*) mem)->name_];
			childNode.cppObjSet.insert(mem);
			childNode.parent = typeNode;
			cppObjToTypeNode_[mem] = &childNode;
			loadType((CppCompound*) mem, &childNode);
		}
		else if(mem->objType_ == CppObj::kTypedef)
		{
			CppTypeTreeNode& childNode = typeNode->children[((CppFunction*) mem)->name_];
			childNode.cppObjSet.insert(mem);
			childNode.parent = typeNode;
			cppObjToTypeNode_[mem] = &childNode;
		}
		else if(mem->objType_ == CppObj::kFunctionPtr)
		{
			CppTypeTreeNode& childNode = typeNode->children[((CppFunctionPtr*) mem)->name_];
			childNode.cppObjSet.insert(mem);
			childNode.parent = typeNode;
			cppObjToTypeNode_[mem] = &childNode;
		}
	}
}

const CppTypeTreeNode* CppProgram::findTypeNode(const std::string& name, const CppTypeTreeNode* typeNode) const
{
	size_t nameBegPos = 0;
	size_t nameEndPos = name.find("::", nameBegPos);
	if(nameEndPos == std::string::npos)
	{
		for(; typeNode != NULL; typeNode = typeNode->parent)
		{
			CppTypeTree::const_iterator itr = typeNode->children.find(name);
			if(itr != typeNode->children.end())
				return &itr->second;
		}
		return NULL;
	}
	else
	{
		std::string nameToLookFor = name.substr(nameBegPos, nameEndPos-nameBegPos);
		typeNode = findTypeNode(name.substr(nameBegPos, nameEndPos-nameBegPos), typeNode);
		if(!typeNode)
			return NULL;
		do
		{
			nameBegPos = nameEndPos + 2;
			nameEndPos = name.find("::", nameBegPos);
			if(nameEndPos == std::string::npos)
				nameEndPos = name.length();
			std::string nameToLookFor = name.substr(nameBegPos, nameEndPos-nameBegPos);
			CppTypeTree::const_iterator itr = typeNode->children.find(nameToLookFor);
			if(itr == typeNode->children.end())
				return NULL;
			typeNode = &itr->second;
		} while (nameEndPos >= name.length());
	}
	return typeNode;
}