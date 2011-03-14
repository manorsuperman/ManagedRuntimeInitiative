/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Sun designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Sun in the LICENSE file that accompanied this code.
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

/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __CLASSDEFINITIONTABLES_H
#define __CLASSDEFINITIONTABLES_H

#include "LETypes.h"
#include "OpenTypeTables.h"

struct ClassDefinitionTable
{
    le_uint16 classFormat;

    le_int32  getGlyphClass(LEGlyphID glyphID) const;
    le_bool   hasGlyphClass(le_int32 glyphClass) const;
};

struct ClassDefFormat1Table : ClassDefinitionTable
{
    TTGlyphID  startGlyph;
    le_uint16  glyphCount;
    le_uint16  classValueArray[ANY_NUMBER];

    le_int32 getGlyphClass(LEGlyphID glyphID) const;
    le_bool  hasGlyphClass(le_int32 glyphClass) const;
};

struct ClassRangeRecord
{
    TTGlyphID start;
    TTGlyphID end;
    le_uint16 classValue;
};

struct ClassDefFormat2Table : ClassDefinitionTable
{
    le_uint16        classRangeCount;
    GlyphRangeRecord classRangeRecordArray[ANY_NUMBER];

    le_int32 getGlyphClass(LEGlyphID glyphID) const;
    le_bool hasGlyphClass(le_int32 glyphClass) const;
};

#endif