/*
 * Copyright 2005 Sun Microsystems, Inc.  All Rights Reserved.
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
 */
package sun.awt.X11;

import sun.misc.Unsafe;
import java.util.logging.*;

class UnsafeXDisposerRecord implements sun.java2d.DisposerRecord {
    private static final Logger log = Logger.getLogger("sun.awt.X11.UnsafeXDisposerRecord");
    private static Unsafe unsafe = XlibWrapper.unsafe;
    final long[] unsafe_ptrs, x_ptrs;
    final String name;
    volatile boolean disposed;
    final Throwable place;
    public UnsafeXDisposerRecord(String name, long[] unsafe_ptrs, long[] x_ptrs) {
        this.unsafe_ptrs = unsafe_ptrs;
        this.x_ptrs = x_ptrs;
        this.name = name;
        if (XlibWrapper.isBuildInternal) {
            place = new Throwable();
        } else {
            place = null;
        }
    }
    public UnsafeXDisposerRecord(String name, long ... unsafe_ptrs) {
        this.unsafe_ptrs = unsafe_ptrs;
        this.x_ptrs = null;
        this.name = name;
        if (XlibWrapper.isBuildInternal) {
            place = new Throwable();
        } else {
            place = null;
        }
    }

    public void dispose() {
        XToolkit.awtLock();
        try {
            if (!disposed) {
                if (XlibWrapper.isBuildInternal && "Java2D Disposer".equals(Thread.currentThread().getName()) && log.isLoggable(Level.WARNING)) {
                    if (place != null) {
                        log.log(Level.WARNING, name + " object was not disposed before finalization!", place);
                    } else {
                        log.log(Level.WARNING, name + " object was not disposed before finalization!");
                    }
                }

                if (unsafe_ptrs != null) {
                    for (long l : unsafe_ptrs) {
                        if (l != 0) {
                            unsafe.freeMemory(l);
                        }
                    }
                }
                if (x_ptrs != null) {
                    for (long l : x_ptrs) {
                        if (l != 0) {
                            if (Native.getLong(l) != 0) {
                                XlibWrapper.XFree(Native.getLong(l));
                            }
                            unsafe.freeMemory(l);
                        }
                    }
                }
                disposed = true;
            }
        } finally {
            XToolkit.awtUnlock();
        }
    }
}
