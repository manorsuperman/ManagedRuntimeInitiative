/*
 * Copyright 2001-2005 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *  
 */
// This file is a derivative work resulting from (and including) modifications
// made by Azul Systems, Inc.  The date of such changes is 2010.
// Copyright 2010 Azul Systems, Inc.  All Rights Reserved.
//
// Please contact Azul Systems, Inc., 1600 Plymouth Street, Mountain View, 
// CA 94043 USA, or visit www.azulsystems.com if you need additional information 
// or have any questions.
#ifndef C1_VALUESET_HPP
#define C1_VALUESET_HPP


#include "bitMap.hpp"
#include "c1_Compilation.hpp"
#include "c1_Instruction.hpp"

// A ValueSet is a simple abstraction on top of a BitMap representing
// a set of Instructions. Currently it assumes that the number of
// instructions is fixed during its lifetime; should make it
// automatically resizable.

class ValueSet: public CompilationResourceObj {
 private:
  BitMap _map;

 public:
  ValueSet();
  
  ValueSet* copy();
  bool contains(Value x);
  void put     (Value x);
  void remove  (Value x);
  bool set_intersect(ValueSet* other);
  void set_union(ValueSet* other);
  void clear   ();
  void set_from(ValueSet* other);
  bool equals  (ValueSet* other);
};

inline ValueSet::ValueSet() : _map(Instruction::number_of_instructions()) {
  _map.clear();
}


inline ValueSet* ValueSet::copy() {
  ValueSet* res = new ValueSet();
  res->_map.set_from(_map);
  return res;
}


inline bool ValueSet::contains(Value x) {
  return _map.at(x->id());
}


inline void ValueSet::put(Value x) {
  _map.set_bit(x->id());
}


inline void ValueSet::remove(Value x) {
  _map.clear_bit(x->id());
}


inline bool ValueSet::set_intersect(ValueSet* other) {
  return _map.set_intersection_with_result(other->_map);
}


inline void ValueSet::set_union(ValueSet* other) {
  _map.set_union(other->_map);
}


inline void ValueSet::clear() {
  _map.clear();
}

inline void ValueSet::set_from(ValueSet* other) {
  _map.set_from(other->_map);
}

inline bool ValueSet::equals(ValueSet* other) {
  return _map.is_same(other->_map);
}

#endif // C1_VALUESET_HPP
