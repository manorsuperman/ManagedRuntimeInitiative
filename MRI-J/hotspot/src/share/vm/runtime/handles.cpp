/*
 * Copyright 1997-2003 Sun Microsystems, Inc.  All Rights Reserved.
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


#include "collectedHeap.hpp"
#include "handles.hpp"
#include "thread.hpp"
#include "universe.hpp"

#include "allocation.inline.hpp"
#include "atomic_os_pd.inline.hpp"

#ifdef ASSERT
objectRef* HandleArea::allocate_handle(oop obj) {
  assert(_handle_mark_nesting > 1, "memory leak: allocating handle outside HandleMark");
  assert(_no_handle_mark_nesting == 0, "allocating handle inside NoHandleMark");
assert(obj->is_oop(),"sanity check");
  return real_allocate_handle(obj);
}

Handle::Handle(Thread* thread, oop obj, bool dummy) {
  assert(thread == Thread::current(), "sanity check");
  if (obj == NULL) {
    _handle = NULL;
  } else {
    _handle = thread->handle_area()->allocate_handle(obj);    
  }
}

objectRef*HandleArea::allocate_handle(objectRef r){
  assert(_handle_mark_nesting > 1, "memory leak: allocating handle outside HandleMark");
  assert(_no_handle_mark_nesting == 0, "allocating handle inside NoHandleMark");
  assert(r.as_oop()->is_oop(), "sanity check");  
return real_allocate_handle(r);
}

Handle::Handle(Thread*thread,objectRef r){
  assert(thread == Thread::current(), "sanity check");
if(r.is_null()){
    _handle = NULL;
  } else {
_handle=thread->handle_area()->allocate_handle(r);
  }
}
#endif


static uintx chunk_oops_do(OopClosure* f, Chunk* chunk, char* chunk_top) {
  objectRef* bottom = (objectRef*) chunk->bottom();
  objectRef* top    = (objectRef*) chunk_top;
  uintx handles_visited = top - bottom;
  assert(top >= bottom && top <= (objectRef*) chunk->top(), "just checking");
  // during GC phase 3, a handle may be a forward pointer that
  // is not yet valid, so loosen the assertion
  while (bottom < top) {
#ifdef ASSERT
    // Making this assert work when LVBs are needed is a pain:
    if ( ! UseLVBs ) { 
      // Actually +UseSBA makes valid stack (non-heap) objects here
      oop o = ALWAYS_UNPOISON_OBJECTREF(*bottom).as_oop();
      assert(Universe::heap()->is_in(o) || Thread::current()->sba_thread()->sba_is_in_or_oldgen((address)o),
"handle should be valid heap address");
    }
#endif // ASSERT
    f->do_oop(bottom++);
  }
  return handles_visited;
}

// Used for debugging handle allocation. 
NOT_PRODUCT(intptr_t _nof_handlemarks=0;)

void HandleArea::oops_do(OopClosure* f) {
  uintx handles_visited = 0;
  // First handle the current chunk. It is filled to the high water mark.
  handles_visited += chunk_oops_do(f, _chunk, _hwm);
  // Then handle all previous chunks. They are completely filled.
  Chunk* k = _first;
  while(k != _chunk) {
    handles_visited += chunk_oops_do(f, k, k->top());
    k = k->next();
  }
  
  // The thread local handle areas should not get very large
  if (TraceHandleAllocation && handles_visited > TotalHandleAllocationLimit) {    
#ifdef ASSERT
    warning(INT64_FORMAT ": Visited in HandleMark : %d", 
      _nof_handlemarks, handles_visited);      
#else
    warning("Visited in HandleMark : %d", handles_visited);
#endif
  }  
  if (_prev != NULL) _prev->oops_do(f);
}

void HandleMark::initialize(Thread* thread) {
  _thread = thread;
  // Save area
  _area  = thread->handle_area();
  // Save current top
  _chunk = _area->_chunk;
  _hwm   = _area->_hwm;
  _max   = _area->_max;
  NOT_PRODUCT(_size_in_bytes = _area->_size_in_bytes;)
  debug_only(_area->_handle_mark_nesting++);
  assert(_area->_handle_mark_nesting > 0, "must stack allocate HandleMarks");
#if defined(ASSERT)
  if (TraceHandleAllocation) {
Atomic::inc_ptr(&_nof_handlemarks);
  }
#endif // defined (ASSERT)

  // Link this in the thread
  set_previous_handle_mark(thread->last_handle_mark());
  thread->set_last_handle_mark(this);
}


HandleMark::~HandleMark() { 
  HandleArea* area = _area;   // help compilers with poor alias analysis
  assert(area == _thread->handle_area(), "sanity check");
  assert(area->_handle_mark_nesting > 0, "must stack allocate HandleMarks" );  
  debug_only(area->_handle_mark_nesting--);
  
  // Debug code to trace the number of handles allocated per mark/
#ifdef ASSERT
  if (TraceHandleAllocation) {
    size_t handles = 0;  
    Chunk *c = _chunk->next();         
    if (c == NULL) {      
      handles = area->_hwm - _hwm; // no new chunk allocated
    } else {
      handles = _max - _hwm;      // add rest in first chunk      
      while(c != NULL) {
        handles += c->length(); 
        c = c->next();
      }    
      handles -= area->_max - area->_hwm; // adjust for last trunk not full
    }
    handles /= sizeof(void *); // Adjust for size of a handle
    if (handles > HandleAllocationLimit) {   
      // Note: _nof_handlemarks is only set in debug mode
      warning(INT64_FORMAT ": Allocated in HandleMark : %d", _nof_handlemarks, handles);
    } 
  }
#endif

  // Delete later chunks
  if( _chunk->next() ) {
    _chunk->next_chop();
  }
  // Roll back arena to saved top markers
  area->_chunk = _chunk;
  area->_hwm = _hwm;
  area->_max = _max;
  NOT_PRODUCT(area->set_size_in_bytes(_size_in_bytes);)
#ifdef ASSERT 
  // clear out first chunk (to detect allocation bugs)
  if (ZapVMHandleArea) {
    memset(_hwm, badHandleValue, _max - _hwm);
  }
  if (TraceHandleAllocation) {
Atomic::dec_ptr(&_nof_handlemarks);
  }
#endif

  // Unlink this from the thread
  _thread->set_last_handle_mark(previous_handle_mark());
}

#ifdef ASSERT

NoHandleMark::NoHandleMark() {
  HandleArea* area = Thread::current()->handle_area();
  area->_no_handle_mark_nesting++;
  assert(area->_no_handle_mark_nesting > 0, "must stack allocate NoHandleMark" );
}


NoHandleMark::~NoHandleMark() {
  HandleArea* area = Thread::current()->handle_area();
  assert(area->_no_handle_mark_nesting > 0, "must stack allocate NoHandleMark" ); 
  area->_no_handle_mark_nesting--;
}


ResetNoHandleMark::ResetNoHandleMark() {
  HandleArea* area = Thread::current()->handle_area();
  _no_handle_mark_nesting = area->_no_handle_mark_nesting;
  area->_no_handle_mark_nesting = 0;
}


ResetNoHandleMark::~ResetNoHandleMark() {
  HandleArea* area = Thread::current()->handle_area();  
  area->_no_handle_mark_nesting = _no_handle_mark_nesting;
}

#endif
