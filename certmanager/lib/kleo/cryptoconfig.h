/*  -*- mode: C++; c-file-style: "gnu" -*-
    cryptoconfig.h

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef CRYPTOCONFIG_H
#define CRYPTOCONFIG_H

#include <kurl.h>

// Start reading this file from the bottom up :)

namespace Kleo {

/**
 * Description of a single config entry
 */
class CryptoConfigEntry {

public:
    /**
	@li basic	This option should always be offered to the user.
	@li advanced	This option may be offered to advanced users.
	@li expert	This option should only be offered to expert users.
	@li invisible	This option should normally never be displayed,
			not even to expert users.
	@li internal	This option is for internal use only.  Ignore it.
        // #### should we even have internal in the API, then?
    */
    enum Level { Level_Basic = 0,
                 Level_Advanced = 1,
                 Level_Expert = 2,
                 Level_Invisible = 3,
                 Level_Internal = 4 };

    /**
       Type of the data
	@li bool	A boolean argument.
	@li string	An unformatted string.
	@li int32	A signed integer number.
	@li uint32	An unsigned integer number.
	@li pathname	A string that describes the pathname of a file.
			The file does not necessarily need to exist.
                        Separated from string so that e.g. a KURLRequester can be used.
	@li url         A URL
    */
    enum DataType { DataType_Bool = 0,
                    DataType_String = 1,
                    DataType_Int = 2,
                    DataType_UInt = 3,
                    DataType_Path = 4,
                    DataType_URL = 5 };

    virtual ~CryptoConfigEntry() {}

    /**
     * @return user-visible description of this entry
     */
    virtual QString description() const = 0;

    /**
     * @return true if the argument is optional
     */
    virtual bool isOptional() const = 0;

    /**
     * @return true if the argument can be given multiple times
     */
    virtual bool isList() const = 0;

    /**
     * @return true if the argument can be changed at runtime
     */
    virtual bool isRuntime() const = 0;

    /**
     * User level
     */
    virtual Level level() const = 0;

    /**
     * Data type
     */
    virtual DataType dataType() const = 0;

    /**
     * Return value as a bool (only valid for Bool datatype)
     */
    virtual bool boolValue() const = 0;

    /**
     * Return value as a string (mostly meaningful for String, Path and URL datatypes)
     * The returned string can be empty (explicitely set to empty) or null (not set).
     */
    virtual QString stringValue() const = 0;

    /**
     * Return value as a signed int
     */
    virtual int intValue() const = 0;

    /**
     * Return value as an unsigned int
     */
    virtual unsigned int uintValue() const = 0;

    /**
     * Return value as a URL (only meaningful for Path and URL datatypes)
     */
    virtual KURL urlValue() const = 0;

    /**
     * Return value as a list of bools (only valid for Bool datatype, if isList())
     */
    virtual QValueList<bool> boolValueList() const = 0;

    /**
     * Return value as a list of strings (mostly meaningful for String, Path and URL datatypes, if isList())
     */
    virtual QStringList stringValueList() const = 0;

    /**
     * Return value as a list of signed ints
     */
    virtual QValueList<int> intValueList() const = 0;

    /**
     * Return value as a list of unsigned ints
     */
    virtual QValueList<unsigned int> uintValueList() const = 0;

    /**
     * Return value as a list of URLs (only meaningful for Path and URL datatypes, if isList())
     */
    virtual KURL::List urlValueList() const = 0;


    /**
     * Set a new boolean value (only valid for Bool datatype)
     *
     * @param runtime If this option is set, the changes will take effect at run-time, as
     * far as this is possible.  Otherwise, they will take effect at the next
     * start of the respective backend programs.
     */
    virtual void setBoolValue( bool, bool /*runtime*/ = true ) = 0;

    /**
     * Set string value (only allowed for String, Path and URL datatypes)
     *
     * @param runtime If this option is set, the changes will take effect at run-time, as
     * far as this is possible.  Otherwise, they will take effect at the next
     * start of the respective backend programs.
     */
    virtual void setStringValue( const QString&, bool /*runtime*/ = true ) = 0;

    /**
     * Set a new signed int value
     *
     * @param runtime If this option is set, the changes will take effect at run-time, as
     * far as this is possible.  Otherwise, they will take effect at the next
     * start of the respective backend programs.
     */
    virtual void setIntValue( int, bool /*runtime*/ = true ) = 0;

    /**
     * Set a new unsigned int value
     *
     * @param runtime If this option is set, the changes will take effect at run-time, as
     * far as this is possible.  Otherwise, they will take effect at the next
     * start of the respective backend programs.
     */
    virtual void setUIntValue( unsigned int, bool /*runtime*/ = true ) = 0;

    /**
     * Set value as a URL (only meaningful for Path (if local) and URL datatypes)
     *
     * @param runtime If this option is set, the changes will take effect at run-time, as
     * far as this is possible.  Otherwise, they will take effect at the next
     * start of the respective backend programs.
     */
    virtual void setURLValue( const KURL&, bool /*runtime*/ = true ) = 0;

    /**
     * Set a new list of boolean values (only valid for Bool datatype, if isList())
     */
    virtual void setBoolValueList( QValueList<bool>, bool /*runtime*/ = true ) = 0;

    /**
     * Set a new string-list value (only allowed for String, Path and URL datatypes, if isList())
     */
    virtual void setStringValueList( const QStringList&, bool /*runtime*/ = true ) = 0;

    /**
     * Set a new list of signed int values
     */
    virtual void setIntValueList( const QValueList<int>&, bool /*runtime*/ = true ) = 0;

    /**
     * Set a new list of unsigned int values
     */
    virtual void setUIntValueList( const QValueList<unsigned int>&, bool /*runtime*/ = true ) = 0;

    /**
     * Set value as a URL list (only meaningful for Path (if all URLs are local) and URL datatypes, if isList())
     */
    virtual void setURLValueList( const KURL::List&, bool /*runtime*/ = true ) = 0;
};

/**
 * Group containing a set of config options
 */
class CryptoConfigGroup {

public:
    virtual ~CryptoConfigGroup() {}

    /**
     * @return user-visible description of this entry
     */
    virtual QString description() const = 0;

    /**
     * User level
     */
    virtual CryptoConfigEntry::Level level() const = 0;

    /**
     * Returns the list of entries that are known by this group.
     *
     * @return list of group entry names.
     **/
    virtual QStringList entryList() const = 0;

    /**
     * @return the configuration object for a given entry in this group
     * The object is owned by CryptoConfigGroup, don't delete it.
     * Groups cannot be nested, so all entries returned here are pure entries, no groups.
     */
    virtual CryptoConfigEntry* entry( const QString& name ) const = 0;
};

/**
 * Crypto config for one component (e.g. gpg-agent, dirmngr etc.)
 */
class CryptoConfigComponent {

public:
    virtual ~CryptoConfigComponent() {}

    /**
     * Return user-visible description of this component
     */
    virtual QString description() const = 0;

    /**
     * Returns the list of groups that are known about.
     *
     * @return list of group names. One of them can be "<nogroup>", which is the group where all
     * "toplevel" options (belonging to no group) are.
     */
    virtual QStringList groupList() const = 0;

    /**
     * @return the configuration object for a given group
     * The object is owned by CryptoConfigComponent, don't delete it.
     */
    virtual CryptoConfigGroup* group( const QString& name ) const = 0;

};

/**
 * Main interface to crypto configuration.
 */
class CryptoConfig {

public:
    virtual ~CryptoConfig() {}

    /**
     * Returns the list of known components (e.g. "gpg-agent", "dirmngr" etc.).
     * Use @ref component() to retrieve more information about each one.
     * @return list of component names.
     **/
    virtual QStringList componentList() const = 0;

    /**
     * @return the configuration object for a given component
     * The object is owned by CryptoConfig, don't delete it.
     */
    virtual CryptoConfigComponent* component( const QString& name ) const = 0;

    /**
     * Convenience method to get hold of a single configuration entry when
     * its component, group and name are known. This can be used to read
     * the value and/or to set a value to it.
     *
     * @return the configuration object for a single configuration entry, 0 if not found.
     * The object is owned by CryptoConfig, don't delete it.
     */
    CryptoConfigEntry* entry( const QString& componentName, const QString& groupName, const QString& entryName ) const {
        const Kleo::CryptoConfigComponent* comp = component( componentName );
        const Kleo::CryptoConfigGroup* group = comp ? comp->group( groupName ) : 0;
        return group ? group->entry( entryName ) : 0;
    }

    /**
     * Tells the CryptoConfig to discard any cached information.
     * Call this to free some memory when you won't be using the object
     * for some time.
     */
    virtual void clear() = 0;
};

};

#endif /* CRYPTOCONFIG_H */
