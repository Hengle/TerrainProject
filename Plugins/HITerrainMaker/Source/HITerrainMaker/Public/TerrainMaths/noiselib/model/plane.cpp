/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// plane.cpp
//
// Copyright (C) 2004 Owen Jacobson
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License (COPYING.txt) for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The developer's email is ojacobson@lionsanctuary.net
//

#include "plane.h"

using namespace noise;
using namespace noise::model;

Plane::Plane ():
  m_pModule (NULL)
{
}

Plane::Plane (const module::Module& module) :
  m_pModule( &module)
{
}

// Told you this was trivial.
double Plane::GetValue (double x, double z) const
{
  assert (m_pModule != NULL);
  
  return m_pModule->GetValue (x, 0, z);
}
