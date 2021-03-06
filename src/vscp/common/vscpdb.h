// vscpdb.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version
// 2 of the License, or (at your option) any later version.
// 
// This file is part of the VSCP (http://www.vscp.org) 
//
// Copyright (C) 2000-2017
// Ake Hedman, Grodans Paradis AB, <akhe@grodansparadis.com>
// 
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this file see the file COPYING.  If not, write to
// the Free Software Foundation, 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//


// VSCP database ordinals etc is defined in this file

#if !defined(VSCPDB__INCLUDED_)
#define VSCPDB__INCLUDED_

//*****************************************************************************
//                              CONFIGURATION
//*****************************************************************************

#define VSCPDB_CONFIG_CREATE "CREATE TABLE 'settings' ("\
	"`vscpd_idx_settings`                               INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`vscpd_dbversion`                                  INTEGER NOT NULL DEFAULT 1,"\
	"`vscpd_loglevel`                                   INTEGER DEFAULT 1,"\
        "'vscpd_clientbuffersize'                           INTEGER DEFAULT 8191,"\
	"`vscpd_runasuser`                                  TEXT DEFAULT '',"\
	"`vscpd_guid`                                       TEXT DEFAULT '00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00',"\
	"`vscpd_servername`                                 TEXT DEFAULT 'THE-VSCP-DAEMON',"\
	"`vscpd_syslog_enable`                              INTEGER DEFAULT 1,"\
        "`vscpd_logdb_enable`                               INTEGER DEFAULT 1,"\
        "`vscpd_db_log_path`                                TEXT DEFAULT '/var/log/vscp/vscpd_log.sqlite3',"\
	"`vscpd_generallogfile_enable`                      INTEGER DEFAULT 1,"\
	"`vscpd_generallogfile_path`                        TEXT DEFAULT '/var/log/vscp/vscpd_log_general',"\
	"`vscpd_securitylogfile_enable`                     INTEGER DEFAULT 1,"\
	"`vscpd_securitylogfile_path`                       TEXT DEFAULT '/var/log/vscp/vscpd_log_security',"\
	"`vscpd_accesslogfile_enable`                       INTEGER DEFAULT 1,"\
	"`vscpd_accesslogFile_path`                         TEXT DEFAULT '/var/log/vscp/vscpd_log_access',"\
	"`vscpd_tcpipinterface_address`                     TEXT DEFAULT '9598',"\
	"`vscpd_multicastinterface_address`                 TEXT DEFAULT '9598',"\
	"`vscpd_mulicastinterface_ttl`                      INTEGER DEFAULT 1,"\
	"`vscpd_udpsimpleinterface_enable`                  INTEGER DEFAULT 0,"\
	"`vscpd_udpsimpleinterface_address`                 TEXT DEFAULT 'udp://:9598',"\
	"`vscpd_dm_db_path`                                 TEXT DEFAULT '/srv/vscp/dm.sqlite3',"\
        "`vscpd_dm_xml_path`                                TEXT DEFAULT '/srv/vscp/dm.xml',"\
	"`vscpd_dm_logging_enable`                          INTEGER DEFAULT 1,"\
	"`vscpd_dm_logging_path`                            TEXT DEFAULT '/var/log/vscp/log_dm',"\
	"`vscpd_dm_logging_level`                           INTEGER DEFAULT 1,"\
	"`vscpd_variables_db_path`                          TEXT DEFAULT '/srv/vscp/variable.sqlite3',"\
        "`vscpd_variables_xml_Path`                         TEXT DEFAULT '/srv/vscp/variable.xml',"\
	"`vscpd_defaultclientbuffersize`                    INTEGER DEFAULT 1024,"\
	"`vscpd_webserver_authentication_enable`            INTEGER DEFAULT 1,"\
	"`vscpd_webserver_rootpath`                         TEXT DEFAULT '/srv/vscp/web',"\
	"`vscpd_webserver_address`                          TEXT DEFAULT '8080',"\
	"`vscpd_webserver_pathcert`                         TEXT DEFAULT '',"\
	"`vscpd_webserver_authdomain`                       TEXT DEFAULT 'mydomain.com',"\
	"`vscpd_webserver_cgiinterpreter`                   TEXT DEFAULT '',"\
	"`vscpd_webserver_cgipattern`                       TEXT DEFAULT '',"\
	"`vscpd_webserver_enabledirectorylistings`          INTEGER DEFAULT 0,"\
	"`vscpd_webserver_hidefilepatterns`                 TEXT DEFAULT '',"\
	"`vscpd_webserver_indexfiles`                       TEXT DEFAULT '',"\
	"`vscpd_webserver_extramimetypes`                   TEXT DEFAULT '',"\
	"`vscpd_webserver_urlrewrites`                      TEXT DEFAULT '',"\
	"`vscpd_webserver_ssipattern`                       TEXT DEFAULT '',"\
	"`vscpd_webserver_runasuser`                        TEXT DEFAULT '',"\
	"`vscpd_webserver_perdirectoryauthfile`             TEXT DEFAULT '',"\
	"`vscpd_webserver_globalauthfile`                   TEXT DEFAULT '',"\
	"`vscpd_webserver_ipacl`                            TEXT DEFAULT '',"\
	"`vscpd_webserver_davdocumentroot`                  TEXT DEFAULT '',"\
	"`vscpd_websocket_enableauth`                       INTEGER DEFAULT 1,"\
	"`vscpd_mqttbroker_enable`                          INTEGER DEFAULT 0,"\
	"`vscpd_mqttbroker_address`                         TEXT DEFAULT '1883',"\
	"`vscpd_coapserver_enable`                          INTEGER DEFAULT 0,"\
	"`vscpd_coapserver_address`                         TEXT DEFAULT 'udp://:5683',"\
	"`vscpd_automation_enable`                          INTEGER DEFAULT 1,"\
	"`vscpd_automation_zone`                            INTEGER DEFAULT 1,"\
	"`vscpd_automation_subzone`                         INTEGER DEFAULT 2,"\
	"`vscpd_automation_longitude`                       REAL DEFAULT 1,"\
	"`vscpd_automation_latitude`                        REAL DEFAULT 1,"\
	"`vscpd_automation_sunrise_enable`                  INTEGER DEFAULT 1,"\
	"`vscpd_automation_sunset_enable`                   INTEGER DEFAULT 1,"\
	"`vscpd_automation_sunsettwilight_enable`           INTEGER DEFAULT 1,"\
	"`vscpd_automation_sunrisetwilight_enable`          INTEGER DEFAULT 1,"\
	"`vscpd_automation_segmentcontrollerevent_enable`   INTEGER DEFAULT 1,"\
	"`vscpd_automation_segmentcontrollerevent_interval` INTEGER DEFAULT 1,"\
	"`vscpd_automation_heartbeatevent_enable`           INTEGER DEFAULT 1,"\
	"`vscpd_automation_heartbeatevent_interval`         INTEGER DEFAULT 60,"\
	"`vscpd_db_data_path`                               TEXT DEFAULT '/srv/vscp/vscp_data.sqlite3',"\
        "`vscpd_db_vscpconf_path`                           TEXT DEFAULT '/srv/vscp/vscpd.sqlite3' "\
        ");";

#define VSCPDB_CONFIG_SET_DEFAULTS  "TODO"        
        

#define VSCPDB_CONFIG_UPDATE_ITEM "UPDATE 'settings' SET %s='%s' WHERE vscpd_idx_settings='%d';"


#define VSCPDB_ORDINAL_CONFIG_ID                                            0
#define VSCPDB_ORDINAL_CONFIG_DBVERSION                                     1
#define VSCPDB_ORDINAL_CONFIG_LOGLEVEL                                      2
#define VSCPDB_ORDINAL_CONFIG_MAXQUEUE                                      3
#define VSCPDB_ORDINAL_CONFIG_RUNASUSER                                     4
#define VSCPDB_ORDINAL_CONFIG_GUID                                          5
#define VSCPDB_ORDINAL_CONFIG_SERVER_NAME                                   6
#define VSCPDB_ORDINAL_CONFIG_SYSLOG_ENABLE                                 7
#define VSCPDB_ORDINAL_CONFIG_LOGDB_ENABLE                                  8
#define VSCPDB_ORDINAL_CONFIG_DB_LOG_PATH                                   9
#define VSCPDB_ORDINAL_CONFIG_GENERALLOGFILE_ENABLE                         10
#define VSCPDB_ORDINAL_CONFIG_GENERALLOGFILE_PATH                           11
#define VSCPDB_ORDINAL_CONFIG_SECURITYLOGFILE_ENABLE                        12
#define VSCPDB_ORDINAL_CONFIG_SECURITYLOGFILE_PATH                          13
#define VSCPDB_ORDINAL_CONFIG_ACCESSLOGFILE_ENABLE                          14
#define VSCPDB_ORDINAL_CONFIG_ACCESSLOGFILE_PATH                            15
#define VSCPDB_ORDINAL_CONFIG_TCPIPINTERFACE_PORT                           16
#define VSCPDB_ORDINAL_CONFIG_MULTICASTINTERFACE_PORT                       17
#define VSCPDB_ORDINAL_CONFIG_MULICASTINTERFACE_TTL                         18
#define VSCPDB_ORDINAL_CONFIG_UDPSIMPLEINTERFACE_ENABLE                     19
#define VSCPDB_ORDINAL_CONFIG_UDPSIMPLEINTERFACE_PORT                       20
#define VSCPDB_ORDINAL_CONFIG_DM_DB_PATH                                    21
#define VSCPDB_ORDINAL_CONFIG_DM_XML_PATH                                   22
#define VSCPDB_ORDINAL_CONFIG_DM_LOGGING_ENABLE                             23
#define VSCPDB_ORDINAL_CONFIG_DM_LOGGING_PATH                               24
#define VSCPDB_ORDINAL_CONFIG_DM_LOGGING_LEVEL                              25
#define VSCPDB_ORDINAL_CONFIG_VARIABLES_DB_PATH                             26
#define VSCPDB_ORDINAL_CONFIG_VARIABLES_XML_PATH                            27
#define VSCPDB_ORDINAL_CONFIG_VSCPD_DEFAULTCLIENTBUFFERSIZE                 28
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_DISABLEAUTHENTICATION               29
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_ROOTPATH                            30
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_PORT                                31
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_PATHCERT                            32
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_AUTHDOMAIN                          33
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_CGIINTERPRETER                      34
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_CGIPATTERN                          35
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_ENABLEDIRECTORYLISTINGS             36
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_HIDEFILEPATTERNS                    37
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_INDEXFILES                          38
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_EXTRAMIMETYPES                      39
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_URLREWRITES                         40
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_SSIPATTERN                          41
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_RUNASUSER                           42
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_PERDIRECTORYAUTHFILE                43
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_GLOBALAUTHFILE                      44
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER__IPACL                              45
#define VSCPDB_ORDINAL_CONFIG_WEBSERVER_DAVDOCUMENTROOT                     46
#define VSCPDB_ORDINAL_CONFIG_WEBSOCKET_ENABLEAUTH                          47
#define VSCPDB_ORDINAL_CONFIG_MQTTBROKER_ENABLE                             48
#define VSCPDB_ORDINAL_CONFIG_MQTTBROKER_PORT                               49
#define VSCPDB_ORDINAL_CONFIG_COAPSERVER_ENABLE                             50
#define VSCPDB_ORDINAL_CONFIG_COAPSERVER_PORT                               51
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_ENABLE                             52
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_ZONE                               53
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SUBZONE                            54
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_LONGITUDE                          55
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_LATITUDE                           56
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SUNRISE_ENABLE                     57
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SUNSET_ENABLE                      58
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SUNSETTWILIGHT_ENABLE              59
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SUNRISETWILIGHT_ENABLE             60
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SEGMENTCONTROLLEREVENT_ENABLE      61
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_SEGMENTCONTROLLEREVENT_INTERVAL    62
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_HEARTBEATEVENT_ENABLE              63
#define VSCPDB_ORDINAL_CONFIG_AUTOMATION_HEARTBEATEVENT_INTERVAL            64
#define VSCPDB_ORDINAL_CONFIG_DB_VSCPDATA_PATH                              65
#define VSCPDB_ORDINAL_CONFIG_DB_VSCPCONF_PATH                              66

//*****************************************************************************
//                                  LOG
//*****************************************************************************

#define VSCPDB_LOG_CREATE "CREATE TABLE 'log' ("\
	"`idx_log`	INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`type`         INTEGER DEFAULT 0,"\
	"`date`         TEXT,"\
	"`level`	INTEGER DEFAULT 0,"\
	"`message`	TEXT"\
        ");";\

#define VSCPDB_ORDINAL_LOG_ID                       0   // 
#define VSCPDB_ORDINAL_LOG_TYPE                     1   // 
#define VSCPDB_ORDINAL_LOG_DATE                     2   // 
#define VSCPDB_ORDINAL_LOG_LEVEL                    3   // 
#define VSCPDB_ORDINAL_LOG_MESSAGE                  4   // 

#define VSCPDB_LOG_INSERT "INSERT INTO 'log' "\
                "(type,date,level,message) VALUES ('%d','%s','%d','%q');"

#define VSCPDB_LOG_COUNT "SELECT COUNT(*) AS nrows FROM log;"

//*****************************************************************************
//                                 USER
//*****************************************************************************

/*
 * Defines users
 * 
 * permissions
 * ===========
 * uuugggooo
 * uuu – user
 * ggg – group  (not used)
 * ooo - other
 *
 * Each group is rw- and other permissions may be added added in front of this.
 * 
 * filter a filter for incoming traffic. Default is open.
 * 
 * permission   - Default user permissions uuugggooo
 * rights       - string with a maximum of six numerical (byte) comma separated 
 *                  fields. Also 'admin', 'driver' and 'user' can be given.
 */

#define VSCPDB_USER_CREATE  "CREATE TABLE 'user' ("\
	"`idx_user`             INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`username`             TEXT NOT NULL UNIQUE,"\
	"`password`             TEXT NOT NULL,"\
	"`fullname`             TEXT NOT NULL,"\
        "`filter`               TEXT,"\
        "`rights`               TEXT DEFAULT '00/00/00/00/00/00/00/00',"\
        "`allowedevents`        TEXT DEFAULT '*:*',"\
        "`allowedremotes`       TEXT DEFAULT '*',"\
	"`note`                 TEXT DEFAULT ''"\
        ");"

#define VSCPDB_USER_INSERT "INSERT INTO 'user' "\
                "(username,password,fullname,filter,rights,allowedevents,allowedremotes,note "\
                " ) VALUES ('%s','%s','%s','%s',%d,'%s','%s','%s' );"

#define VSCPDB_USER_UPDATE "UPDATE 'user' "\
                "SET username='%s',password='%s',fullname='%s',filter='%s',rights='%s',allowedevents='%s',allowedremotes='%s',note='%s' "\
                " WHERE idx_user='%ld'"

#define VSCPDB_USER_ALL "SELECT * from 'user'"

#define VSCPDB_USER_CHECK_USER "SELECT idx_user from 'user' WHERE username='%s'"

#define VSCPDB_USER_CHECK_USER_ID "SELECT username FROM 'user' where idx_user='%lu'"

#define VSCPDB_USER_DELETE_USERNAME "DELETE FROM 'user' where username='%s'"

#define VSCPDB_ORDINAL_USER_ID                      0   // 
#define VSCPDB_ORDINAL_USER_USERNAME                1   // 
#define VSCPDB_ORDINAL_USER_PASSWORD                2   // 
#define VSCPDB_ORDINAL_USER_FULLNAME                3   // 
#define VSCPDB_ORDINAL_USER_FILTER                  4   //
#define VSCPDB_ORDINAL_USER_RIGHTS                  5   //
#define VSCPDB_ORDINAL_USER_ALLOWED_EVENTS          6   //
#define VSCPDB_ORDINAL_USER_ALLOWED_REMOTES         7   //
#define VSCPDB_ORDINAL_USER_NOTE                    8   // 


//*****************************************************************************
//                               DRIVER
//*****************************************************************************

/*
 * List drivers (Level I and Level II) used by the system
 * bEnable True for driver to be loaded.
 * level Currently Level I (0) or Level II (1)
 * name Name for driver.
 * link_to_guid Pointer to GUID for this driver. All drivers must have a GUID.
 * configuration Semicolon separated configuration string for driver.
 * path Path to driver dl/dll
 * flags Flags for driver functionality
 * translation Semicolon separated list. Translate Level I events to Level II 
 *      (measurements float/string)
 * note Driver notations and/or information.
 */

#define VSCPDB_DRIVER_CREATE "CREATE TABLE `driver` ("\
	"`idx_driver`       INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`bEnable`          INTEGER NOT NULL,"\
	"`level`            INTEGER NOT NULL DEFAULT 0,"\
	"`name`             TEXT NOT NULL,"\
	"`link_to_guid`     INTEGER,"\
	"`configuration`    TEXT,"\
	"`path`             TEXT,"\
	"`flags`            INTEGER,"\
        "`translation`      TEXT,"\
	"`note`             TEXT"\
        ");"

#define VSCPDB_ORDINAL_DRIVER_ID                    0   // 
#define VSCPDB_ORDINAL_DRIVER_ENABLE                1   //
#define VSCPDB_ORDINAL_DRIVER_LEVEL                 2   //
#define VSCPDB_ORDINAL_DRIVER_NAME                  3   //
#define VSCPDB_ORDINAL_DRIVER_LINK_TO_GUID          4   //
#define VSCPDB_ORDINAL_DRIVER_CONFIGURATION         5   //
#define VSCPDB_ORDINAL_DRIVER_PATH                  6   //
#define VSCPDB_ORDINAL_DRIVER_FLAGS                 7   //
#define VSCPDB_ORDINAL_DRIVER_TRANSLATION           8   //
#define VSCPDB_ORDINAL_DRIVER_NOTE                  9   //


//*****************************************************************************
//                     GUID (Discovery)    . DB:discovery
//*****************************************************************************

/*
 * GUID table
 * 
 * type - describes what this GUID is describing, for example an interface, a node etc.
 * type = 0 - Common GUID.
 * type = 1 - Interface on this machine,
 * type = 2 - Level I hardware. Lives on one of the interfaces of this daemon.
 * type = 3 - Level II hardware. Lives somewhere given by address.
 * Type = 4 – Location.
 * 
 * GUID - GUID for the type. Level I hardware use the proxy GUID.
 * 
 * date - date time when discovered in ISO format. YY-MM-DDTHH:MM:SS
 * 
 * name - Max 64 byte name
 * 
 * link_to_mdf - For a hardware device. 
 * 
 * address - is IPv4/IPv6/BT-UID or other address for a Level II hardware type
 * 
 * capabilities - see CLASS2.PROTOCOL, Type=20   A 8 byte comma separated list with
 *                the bytes of the capability 64-bit code.
 * 
 * nonstandard - is one or more nonstandard info as described for CLASS2.PROTOCOL, Type=20
 * 
 */

#define VSCPDB_GUID_CREATE "CREATE TABLE `guid` ("\
	"`idx_guid`	INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`type`         INTEGER NOT NULL,"\
	"`guid`         TEXT DEFAULT '00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00',"\
        "`date`         TEXT,"\
        "`name`         TEXT,"\
        "`link_to_mdf`	INTEGER,"\
        "`address`      TEXT,"\
        "`capabilities` TEXT,"\
        "`nonstandard`  TEXT,"\
        "`description`	TEXT "\
        ");"

#define VSCPDB_ORDINAL_GUID_ID                      0   // 
#define VSCPDB_ORDINAL_GUID_TYPE                    1   //
#define VSCPDB_ORDINAL_GUID_GUID                    2   //
#define VSCPDB_ORDINAL_GUID_DATE                    3   //
#define VSCPDB_ORDINAL_GUID_NAME                    4   //
#define VSCPDB_ORDINAL_GUID_LINK_TO_MDF             5   //
#define VSCPDB_ORDINAL_GUID_ADDRESS                 6   //
#define VSCPDB_ORDINAL_GUID_CAPABILITIES            7   //
#define VSCPDB_ORDINAL_GUID_NONSTANDARD             8   //
#define VSCPDB_ORDINAL_GUID_DESCRIPTION             9   //
                
                
//*****************************************************************************
//                               LOCATION
//*****************************************************************************

/*
 * Defines locations.
 * A GUID can also be used to identify a location. If so link_to_guid points to it.
 */

#define VSCPDB_LOCATION_CREATE "CREATE TABLE `location` ("\
	"`idx_location`         INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`link_to_zone`         INTEGER,"\
	"`link_to_subzone`	INTEGER,"\
	"`link_to_guid`         INTEGER,"\
        "`name`                 TEXT NOT NULL UNIQUE,"\
        "`description`	I       TEXT "\
        ");"

#define VSCPDB_ORDINAL_LOCATION_ID                  0   //            
#define VSCPDB_ORDINAL_LOCATION_LINK_TO_ZONE        1   //
#define VSCPDB_ORDINAL_LOCATION_LINK_TO_SUBZONE     2   //
#define VSCPDB_ORDINAL_LOCATION_LINK_TO_GUID        3   //
#define VSCPDB_ORDINAL_LOCATION_NAME                4   //
#define VSCPDB_ORDINAL_LOCATION_DESCRIPTION         5   //                
                
                
//*****************************************************************************
//                               MDF_CACHE
//*****************************************************************************

/*
 * Loaded MDF's are cached. This record points to the loaded MDF
 * 
 * mdf      - The MDF file content.
 * picture  - Picture of device
 * date     - When the MDF was fetched.
 * guid     - GUID for the device.
 */
#define VSCPDB_MDF_CACHE_CREATE "CREATE TABLE 'mdf_cache' ("\
	"`idx_mdf`	INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`url`          TEXT NOT NULL UNIQUE,"\
	"`mdf`          TEXT NOT NULL,"\
	"`picture`	BLOB,"\
	"`date`         TEXT,"\
        "`guid`         TEXT DEFAULT '00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00'"\
        ");"
                
#define VSCPDB_ORDINAL_MDF_CACHE_ID                 0   //
#define VSCPDB_ORDINAL_MDF_CACHE_URL                1   //
#define VSCPDB_ORDINAL_MDF_CACHE_FILE_PATH          2   //
#define VSCPDB_ORDINAL_MDF_CACHE_PICTURE_PATH       3   //
#define VSCPDB_ORDINAL_MDF_CACHE_DATE               4   //
#define VSCPDB_ORDINAL_MDF_CACHE_GUID               5   //

//*****************************************************************************
//                                SIMPLEUI
//*****************************************************************************

/*
 * Defines a simple UI
 */

#define VSCPDB_SIMPLE_UI_CREATE "CREATE TABLE 'simpleui' ("\
	"`idx_simpleui`     INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`name`             TEXT NOT NULL UNIQUE,"\
	"`link_to_ower`     INTEGER NOT NULL,"\
	"`permission`       INTEGER DEFAULT 777,"\
        "`comment`          TEXT"\
        ");";

 
#define VSCPDB_ORDINAL_SIMPLE_UI_NAME               1   //
#define VSCPDB_ORDINAL_SIMPLE_UI_LINK_TO_OWNER      2   //  
#define VSCPDB_ORDINAL_SIMPLE_UI_PERMISSION         3   // 
#define VSCPDB_ORDINAL_SIMPLE_UI_COMMENT            4   //


//*****************************************************************************
//                              SIMPLEUI_ITEM
//*****************************************************************************

/*
 * Defines a simple UI item
 * param_... defines contents for the row type
 */
                
#define VSCPDB_SIMPLE_UI_ITEM_CREATE "CREATE TABLE `simpleui_item` ("\
	"`idx_simpleui_item`	INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`link_to_simpleui`	INTEGER NOT NULL,"\
        "`param_left`           TEXT NOT NULL,"\
        "`param_middle`         TEXT NOT NULL,"\
        "`param_right`          TEXT NOT NULL,"\
	"`page`                 INTEGER,"\
	"`sortorder`            INTEGER,"\
	"`rowtype`              INTEGER DEFAULT 0"\
        ");"

#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_ID                0   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_LINK_TO_SIMPLEUI  1   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_PARAM_LEFT        2   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_PARAM_MIDDLE      3   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_PARAM_RIGHT       4   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_PAGE              5   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_SORT_ORDER        6   //
#define VSCPDB_ORDINAL_SIMPLE_UI_ITEM_ROW_TYPE          7   //


//*****************************************************************************
//                                  ZONE
//*****************************************************************************

#define VSCPDB_ZONE_CREATE "CREATE TABLE 'zone' ("\
	"`idx_zone`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"\
	"`name`         TEXT NOT NULL,"\
	"`description`	TEXT"\
        ");"

#define VSCPDB_ORDINAL_ZONE_ID                   0   //
#define VSCPDB_ORDINAL_ZONE_NAME                 1   //
#define VSCPDB_ORDINAL_ZONE_DESCRIPTION          2   //

//*****************************************************************************
//                                 SUBZONE
//*****************************************************************************

#define VSCPDB_SUBZONE_CREATE "CREATE TABLE `subzone` ("\
	"`idx_subzone`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"\
	"`name`         TEXT NOT NULL,"\
	"`description`	TEXT"\
        ");"

#define VSCPDB_ORDINAL_SUBZONE_ID               0   //
#define VSCPDB_ORDINAL_SUBZONE_NAME             1   //
#define VSCPDB_ORDINAL_SUBZONE_DESCRIPTION      2   //



//*****************************************************************************
//                              USERDEF_TABLE
//*****************************************************************************

/*
 * User defined tables with diagram hints
 * xname - Text on xaxis
 * yname - Text on yaxis
 * title - Text for diagram title
 * xfield - fields for data
 * note - Text to display as note on diagram.
 * tabletype - code for type of table
 * param - parameter for table type
 * size - A specific size for a table with a defined size (round robin).
 * sql_create - SQL expression to use to create table
 * sql_insertvalue - SQL expression to insert value. The value should be set as 
 *  $value in the SQL expression and will be substituted with the real value as 
 *  a floating point value.
 * decription - User description for table
 */

#define VSCPDB_USERDEF_TABLE_CREATE "CREATE TABLE 'userdef_table' ("\
	"`idx_table`        INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
	"`name`             TEXT NOT NULL,"\
	"`link_to_owner`    INTEGER NOT NULL,"\
	"`permission`       INTEGER NOT NULL,"\
	"`xname`            BLOB NOT NULL,"\
	"`yname`            TEXT NOT NULL,"\
	"`title`            REAL NOT NULL,"\
	"`xfield`           TEXT NOT NULL,"\
	"`yfield`           TEXT NOT NULL,"\
	"`note`             TEXT,"\
        "`tabletype`        INTEGER,"\
        "`param`            TEXT,"\
	"`size`             INTEGER DEFAULT 0,"\
	"`sql_create`       TEXT NOT NULL,"\
	"`sql_insertvalue`  TEXT NOT NULL,"\
        "`description`      TEXT NOT NULL"\
        ");"

#define VSCPDB_ORDINAL_USERDEF_TABLE_ID                 0   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_NAME               1   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_LINK_TO_OWNER      2   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_PERMISSION         3   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_XNAME              4   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_YNAME              5   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_TITLE              6   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_XFIELD             7   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_YFIELD             8   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_NOTE               9   //
#define VSCPDB_ORDINAL_USERDEF_TABLE_TABLETYPE          10  //
#define VSCPDB_ORDINAL_USERDEF_TABLE_PARAM              11  //
#define VSCPDB_ORDINAL_USERDEF_TABLE_SIZE               12  //
#define VSCPDB_ORDINAL_USERDEF_TABLE_SQL_CREATE         13  //
#define VSCPDB_ORDINAL_USERDEF_TABLE_SQL_INSERTVALUE    14  //
#define VSCPDB_ORDINAL_USERDEF_TABLE_DESCRIPTION        15  //




// ----------------------------------------------------------------------------




//*****************************************************************************
//                                VARIABLE
//*****************************************************************************

//
// bstock - True for a stock variable. A stock variable is virtual but is 
//          in the database for sorting, searching and listing of variables.
//              lastchange is always set to program start
//              value is always  empty
//              name set to correct name
//              type set to correct type
//              bpersistent set to false
//              link_to-user = 0 (admin)
//              permission depends on the actual variable
//          ONLY!!!!  PERSISTENT  !!!!   no use for persistent variables
//

#define VSCPDB_VARIABLE_CREATE  "CREATE TABLE 'variable' ("\
                        "'idx_variable'     INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
                        "'bstock'           INTEGER NOT NULL DEFAULT 0,"\
                        "'lastchange'       TEXT NOT NULL,"\
                        "'name'             TEXT NOT NULL UNIQUE,"\
                        "'type'             INTEGER NOT NULL DEFAULT 0,"\
                        "'value'            TEXT NOT NULL,"\
                        "'bPersistent'      INTEGER NOT NULL DEFAULT 0,"\
                        "'link_to_user'     INTEGER NOT NULL,"\
                        "'permission'       INTEGER NOT NULL DEFAULT 777,"\
                        "'note'	TEXT"\
                        ");"

#define VSCPDB_VARIABLE_UPDATE "UPDATE 'variable' "\
                                        "SET lastchange='%s', "\
                                        "name='%q', "\
                                        "type='%d', "\
                                        "value='%q', "\
                                        "bPersistent='%d', "\
                                        "link_to_user='%d', "\
                                        "permission='%d', "\
                                        "note='%q' "\
                                        "WHERE idx_variable='%ld';"

#define VSCPDB_VARIABLE_INSERT "INSERT INTO 'variable' "\
                        "(lastchange,name,type,value,bPersistent,link_to_user,permission,note) "\
                        "VALUES ('%s','%s', '%d','%q','%d','%d','%d','%q');"

#define VSCPDB_VARIABLE_FIND_ALL "SELECT * FROM 'variable'"

#define VSCPDB_VARIABLE_FIND_FROM_NAME "SELECT * FROM 'variable' WHERE name='%s'"

#define VSCPDB_VARIABLE_WITH_ID  "DELETE FROM 'variable' WHERE idx_variable='%ld';"

#define VSCPDB_ORDINAL_VARIABLE_ID              0   //
#define VSCPDB_ORDINAL_VARIABLE_BSTOCK          1   //
#define VSCPDB_ORDINAL_VARIABLE_LASTCHANGE      2   //
#define VSCPDB_ORDINAL_VARIABLE_NAME            3   // 
#define VSCPDB_ORDINAL_VARIABLE_TYPE            4   // 
#define VSCPDB_ORDINAL_VARIABLE_VALUE           5   // 
#define VSCPDB_ORDINAL_VARIABLE_PERSISTENT      6   // 
#define VSCPDB_ORDINAL_VARIABLE_LINK_TO_USER    7   //  
#define VSCPDB_ORDINAL_VARIABLE_PERMISSION      8   // 
#define VSCPDB_ORDINAL_VARIABLE_NOTE            9   // 





// ----------------------------------------------------------------------------





//*****************************************************************************
//                                     DM
//*****************************************************************************



#define VSCPDB_DM_CREATE  "CREATE TABLE 'dm' ("\
	"`idx_dm`                       INTEGER NOT NULL PRIMARY KEY UNIQUE,"\
        "`GroupID`                      TEXT NOT NULL,"\
	"`bEnable`                      INTEGER NOT NULL DEFAULT 0,"\
	"`maskPriority`                 INTEGER NOT NULL DEFAULT 0,"\
	"`maskClass`                    NUMERIC NOT NULL DEFAULT 0,"\
	"`maskType`                     INTEGER NOT NULL DEFAULT 0,"\
	"`maskGUID`                     TEXT NOT NULL DEFAULT '00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00',"\
	"`filterPriority`               INTEGER NOT NULL DEFAULT 0,"\
	"`filterClass`                  INTEGER NOT NULL DEFAULT 0,"\
	"`filterType`                   INTEGER NOT NULL DEFAULT 0,"\
	"`filterGUID`                   TEXT NOT NULL,"\
	"`allowedFrom`                  TEXT NOT NULL,"\
	"`allowedTo`                    TEXT NOT NULL,"\
	"`allowedMonday`                INTEGER NOT NULL,"\
	"`allowedTuesday`               INTEGER NOT NULL,"\
	"`allowsWednesday`              INTEGER NOT NULL,"\
	"`allowedThursday`              INTEGER NOT NULL,"\
	"`allowedFriday`                INTEGER NOT NULL,"\
	"`allowedSaturday`              NUMERIC NOT NULL,"\
	"`allowedSunday`                INTEGER NOT NULL,"\
	"`allowedTime`                  TEXT NOT NULL,"\
	"`bCheckIndex`                  INTEGER NOT NULL,"\
	"`index`                        TEXT NOT NULL,"\
	"`bCheckZone`                   TEXT NOT NULL,"\
	"`zone`                         INTEGER NOT NULL,"\
	"`bCheckSubZone`                INTEGER NOT NULL,"\
	"`subzone`                      INTEGER NOT NULL,"\
	"`bCheckMeasurementIndex`	INTEGER NOT NULL,"\
	"`meaurementIndex`              INTEGER NOT NULL,"\
	"`actionCode`                   TEXT NOT NULL,"\
	"`actionParameter`              NUMERIC NOT NULL,"\
        "`bCheckMeasurementValue`	INTEGER NOT NULL,"\
	"`measurementValue`             REAL,"\
	"`measurementUnit`              INTEGER,"\
	"`measurementCompare`           INTEGER,"\
	"`comment`                      TEXT"\
    ");"

#define VSCPDB_DM_INSERT "INSERT INTO 'dm' "\
                "(GroupID,bEnable,maskPriority,maskClass,maskType,maskGUID,filterPriority,filterClass,filterType,filterGUID,"\
                "allowedStart,allowedEnd,allowedMonday,allowedTuesday,allowsWednesday,allowedThursday,allowedFriday,allowedSaturday,"\
                "allowedSunday,allowedTime,bCheckIndex,index,bCheckZone,zone,bCheckSubZone,subzone,bCheckMeasurementIndex,"\
                "meaurementIndex,actionCode,actionParameter,bCheckMeasurementValue,measurementValue,measurementUnit,measurementCompare,comment"\
                " )VALUES ('%s','%d','%d','%d','%d','%s','%d','%d','%d','%s',"\
                "'%s','%s','%d','%d','%d','%d','%d','%d','%d',"\
                "'%d','%s','%d','%d','%d','%d','%d','%d','%d','%d','%d','%s','%d',%f','%d','%s'"\
                ");"
                        
#define VSCPDB_DM_UPDATE "UPDATE 'dm' "\
                "SET GroupID='%s',bEnable='%d',maskPriority='%d',maskClass=%d',maskType=%d',maskGUID=%s',filterPriority=%d',filterClass=%d',filterType=%d',filterGUID=%s',"\
                "allowedStart=%s',allowedEnd=%s',allowedMonday='%d',allowedTuesday='%d',allowsWednesday='%d',allowedThursday='%d',allowedFriday='%d',allowedSaturday='%d',"\
                "allowedSunday,allowedTime,bCheckIndex,index,bCheckZone,zone,bCheckSubZone,subzone,bCheckMeasurementIndex,"\
                "meaurementIndex='%d',actionCode='%d',actionParameter='%s',bCheckMeasurementValue=%d,measurementValue='%f',measurementUnit='%d',measurementCompare='%d'"\
                " WHERE idx_dm='%d';" 

#define VSCPDB_DM_UPDATE_ITEM "UPDATE 'dm' SET ( %s='%s' ) WHERE id='%d' ;"

#define VSCPDB_ORDINAL_DM_ID                        0   // 
#define VSCPDB_ORDINAL_DM_ENABLE                    1   //
#define VSCPDB_ORDINAL_DM_GROUPID                   2   //
#define VSCPDB_ORDINAL_DM_MASK_PRIORITY             3   //
#define VSCPDB_ORDINAL_DM_MASK_CLASS                4   //
#define VSCPDB_ORDINAL_DM_MASK_TYPE                 5   //
#define VSCPDB_ORDINAL_DM_MASK_GUID                 6   //
#define VSCPDB_ORDINAL_DM_FILTER_PRIORITY           7   //
#define VSCPDB_ORDINAL_DM_FILTER_CLASS              8   //
#define VSCPDB_ORDINAL_DM_FILTER_TYPE               9   //
#define VSCPDB_ORDINAL_DM_FILTER_GUID               10   //
#define VSCPDB_ORDINAL_DM_ALLOWED_START             11   //
#define VSCPDB_ORDINAL_DM_ALLOWED_END               12   //
#define VSCPDB_ORDINAL_DM_ALLOWED_MONDAY            13   //
#define VSCPDB_ORDINAL_DM_ALLOWED_TUESDAY           14   //
#define VSCPDB_ORDINAL_DM_ALLOWED_WEDNESDAY         15   //
#define VSCPDB_ORDINAL_DM_ALLOWED_THURSDAY          16   //
#define VSCPDB_ORDINAL_DM_ALLOWED_FRIDAY            17   //
#define VSCPDB_ORDINAL_DM_ALLOWED_SATURDAY          18   //
#define VSCPDB_ORDINAL_DM_ALLOWED_SUNDAY            19   //
#define VSCPDB_ORDINAL_DM_ALLOWED_TIME              20   //
#define VSCPDB_ORDINAL_DM_CHECK_INDEX               21   //
#define VSCPDB_ORDINAL_DM_INDEX                     22   //
#define VSCPDB_ORDINAL_DM_CHECK_ZONE                23   //
#define VSCPDB_ORDINAL_DM_ZONE                      24   //
#define VSCPDB_ORDINAL_DM_CHECK_SUBZONE             25   //
#define VSCPDB_ORDINAL_DM_SUBZONE                   26   //
#define VSCPDB_ORDINAL_DM_CHECK_MEASUREMENT_INDEX   27   //
#define VSCPDB_ORDINAL_DM_MEASUREMENT_INDEX         28   //
#define VSCPDB_ORDINAL_DM_ACTIONCODE                29   //
#define VSCPDB_ORDINAL_DM_ACTIONPARAMETER           30   //
#define VSCPDB_ORDINAL_DM_MEASUREMENT_CHECK_VALUE   31   //
#define VSCPDB_ORDINAL_DM_MEASUREMENT_VALUE         32   //
#define VSCPDB_ORDINAL_DM_MEASUREMENT_UNIT          33   //
#define VSCPDB_ORDINAL_DM_MEASUREMENT_COMPARE       34   //
#define VSCPDB_ORDINAL_DM_MEASUREMENT_COMMENT       35   //




#endif  // compile

        