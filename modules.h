/*
 * Taken from QtExt
 *
 * Copyright (C) 2008 Bernhard Rosenkraenzer <bero@arklinux.org>
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the license, or if, and only if, any part of
 * version 2 is ruled invalid in a court of law, any later
 * version published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 */
#ifndef __MODULES__
#define __MODULES__ 1

#include <QString>

/** @class Modules Modules <Modules>
 * Tools for working with Linux loadable kernel modules
 */
class Modules {
public:
	/**
	 * Get the instance of the Modules object
	 */
	static Modules *	instance();
	/**
	 * Load a kernel module
	 * @param name Name (base name or full path name) of module to be loaded
	 * @param args Arguments to be passed to the module
	 */
	bool			load(QString name, QString args=QString::null);
	/**
	 * Load a kernel module and everything it depends on
	 * @param name Name (base name or full path name) of module to be loaded
	 * @param args Arguments to be passed to the module
	 */
	bool			loadWithDeps(QString name, QString args=QString::null);
	/**
	 * Unload a kernel module
	 * @param name Name of the module
	 * @param force If @c true, unload the module even if it seems to be in use
	 * @param wait If @c true, don't return until the module has been unloaded
	 */
	bool			unload(char const * const name, bool force=false, bool wait=true);
	/**
	 * Deterine the filename of a module
	 * @param name base name of the module (e.g. 8139too)
	 * @return Full path name of the module (e.g. /lib/modules/2.6.31-1arksmp/kernel/drivers/net/8139too.ko)
	 */
	QString			filename(QString const &name);
	/**
	 * Deterine the name of a module
	 * @param file Full path name of the file (e.g. /lib/modules/2.6.31-1arksmp/kernel/drivers/net/8139too.ko)
	 * @return Base name (e.g. 8139too)
	 */
	QString			modname(QString const &file);
private:
	Modules();
protected:
	QString			_modDir;
	static Modules *	_instance;
};

#endif
