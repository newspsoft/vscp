// tcpipclientthread.cpp
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

#ifdef WIN32
#include <winsock2.h>
#endif

//#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/defs.h"
#include "wx/app.h"
#include <wx/listimpl.cpp>
#include <wx/tokenzr.h>
#include <wx/stdpaths.h>
#include <time.h>

#ifndef DWORD
#define DWORD unsigned long
#endif

#include <vscp.h>
#include "tcpipclientthread.h"
#include <canal_win32_ipc.h>
#include <canal_macro.h>
#include <canal.h>
#include <vscphelper.h>
#include <dllist.h>
#include <mongoose.h>
#include <version.h>
#include <controlobject.h>


///////////////////////////////////////////////////
//                 GLOBALS
///////////////////////////////////////////////////

extern CControlObject *gpobj;


//WX_DEFINE_LIST(TCPCLIENTS);


///////////////////////////////////////////////////////////////////////////////
// VSCPWebServerThread
//
// This thread listens for connection on a TCP socket and starts a new thread
// to handle client requests
//

VSCPClientThread::VSCPClientThread()
: wxThread(wxTHREAD_JOINABLE)
{
    m_bQuit = false;
    m_pCtrlObject = NULL;
}


VSCPClientThread::~VSCPClientThread()
{
    ;
}


///////////////////////////////////////////////////////////////////////////////
// Entry
//

void *VSCPClientThread::Entry()
{
    // Check pointers
    if ( NULL == m_pCtrlObject ) return NULL;

    mg_mgr_init( &m_pCtrlObject->m_mgrTcpIpServer, m_pCtrlObject );

    // Construct bind interface address
    //[PROTO://][IP_ADDRESS]:PORT where host part is optional
    m_pCtrlObject->m_strTcpInterfaceAddress.Trim();
    m_pCtrlObject->m_strTcpInterfaceAddress.Trim( false );
    wxStringTokenizer tkz( m_pCtrlObject->m_strTcpInterfaceAddress, _(" ") );
    while ( tkz.HasMoreTokens() ) {

        wxString str = tkz.GetNextToken();
        str.Trim();
        str.Trim( false );
        if ( 0 == str.Length() ) continue;

        // Bind to this interface
        mg_bind( &m_pCtrlObject->m_mgrTcpIpServer,
                    (const char *)str.mbc_str(),
                    VSCPClientThread::ev_handler );

    }

    m_pCtrlObject->logMsg(_T("TCP Client: Thread started.\n") );

    while ( !TestDestroy() && !m_bQuit ) {
        mg_mgr_poll( &m_pCtrlObject->m_mgrTcpIpServer, 50 );
        Yield();
    }

    // release the server
    mg_mgr_free( &m_pCtrlObject->m_mgrTcpIpServer );

    m_pCtrlObject->logMsg( _T( "TCP ClientThread: Quit.\n" )  );

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// OnExit
//

void VSCPClientThread::OnExit()
{
    ;
}

///////////////////////////////////////////////////////////////////////////////
// ev_handler
//

void VSCPClientThread::ev_handler( struct mg_connection *conn,
                                    int ev,
                                    void *pUser)
{
    char rbuf[ 2048 ];
    int pos4lf;

    CControlObject *pCtrlObject = NULL;
    CClientItem *pClientItem = NULL;

    if ( NULL == conn ) {
        return;
    }

    pClientItem = ( CClientItem * )conn->user_data;

    if ( ( NULL == conn->mgr ) || ( NULL == conn->mgr->user_data ) ) {
        conn->flags |= MG_F_CLOSE_IMMEDIATELY; // Close connection
        return;
    }
    pCtrlObject = ( CControlObject * )conn->mgr->user_data;

    switch (ev) {

        case MG_EV_CONNECT: // connect() succeeded or failed. int *success_status
            pCtrlObject->logMsg(_("TCP Client: Connect.\n") );
            break;

        case MG_EV_ACCEPT:	// New connection accept()-ed. union socket_address *remote_addr
            {
                pCtrlObject->logMsg(_("TCP Client: -- Accept.\n") );

                // We need to create a clientobject and add this object to the list
                pClientItem = new CClientItem;
                if ( NULL == pClientItem ) {
                    pCtrlObject->logMsg ( _( "[TCP/IP Client] Unable to allocate memory for client.\n" )  );
                    conn->flags |= MG_F_CLOSE_IMMEDIATELY;	// Close connection
                    return;
                }

                // save the client item
                conn->user_data = pClientItem;

                // This is now an active Client
                pClientItem->m_bOpen = true;
                pClientItem->m_type =  CLIENT_ITEM_INTERFACE_TYPE_CLIENT_TCPIP;
                pClientItem->m_strDeviceName = _("Remote TCP/IP Client. Started at ");
                wxDateTime now = wxDateTime::Now();
                pClientItem->m_strDeviceName += now.FormatISODate();
                pClientItem->m_strDeviceName += _(" ");
                pClientItem->m_strDeviceName += now.FormatISOTime();

                // Add the client to the Client List
                pCtrlObject->m_wxClientMutex.Lock();
                pCtrlObject->addClient( pClientItem );
                pCtrlObject->m_wxClientMutex.Unlock();

                // Clear the filter (Allow everything )
                vscp_clearVSCPFilter( &pClientItem->m_filterVSCP );

                // Send welcome message
                wxString str = _(MSG_WELCOME);
                str += _("Version: ");
                str += _(VSCPD_DISPLAY_VERSION);
                str += _("\r\n");
                str += _(VSCPD_COPYRIGHT);
                str += _("\r\n");
                str += _(MSG_OK);
                mg_send( conn, (const char*)str.mbc_str(), str.Length() );

                pCtrlObject->logMsg(_("TCP Client: Ready to serve client.\n"),
                                        DAEMON_LOGMSG_DEBUG);
            }
            break;

        case MG_EV_CLOSE:

            // Close client
            pClientItem->m_bOpen = false;
            conn->flags |= MG_F_CLOSE_IMMEDIATELY;   // Close connection

            // Remove the client from the Client List
            pCtrlObject->m_wxClientMutex.Lock();
            pCtrlObject->removeClient( pClientItem );
            pCtrlObject->m_wxClientMutex.Unlock();
            // Remove client item
            conn->user_data = NULL;
            break;

        case MG_EV_RECV:

            if ( NULL == pClientItem ) {
                pCtrlObject->logMsg( _( "[TCP/IP Client] Remote client died\n" )  );
                conn->flags |= MG_F_CLOSE_IMMEDIATELY; // Close connection
                return;
            }

            if ( sizeof( rbuf ) < conn->recv_mbuf.len ) {
                pCtrlObject->logMsg( _("[TCP/IP Client] Received io->buf size exceeds limit.\n" )  );
                conn->flags |= MG_F_CLOSE_IMMEDIATELY; // Close connection
                return;
            }

            // Read new data
            memset( rbuf, 0, sizeof( rbuf ) );
            memcpy( rbuf, conn->recv_mbuf.buf, conn->recv_mbuf.len );
            
            mbuf_remove( &conn->recv_mbuf, conn->recv_mbuf.len );
            pClientItem->m_readBuffer += wxString::FromUTF8( rbuf );

            // Check if command already in buffer
            while ( wxNOT_FOUND != ( pos4lf = pClientItem->m_readBuffer.Find ( (const char)0x0a ) ) ) {
                wxString strCmdGo = pClientItem->m_readBuffer.Mid( 0, pos4lf );
                pCtrlObject->getTCPIPServer()->CommandHandler( conn,
                                                                pCtrlObject,
                                                                strCmdGo );
                pClientItem->m_readBuffer =
                        pClientItem->m_readBuffer.Right( pClientItem->m_readBuffer.Length()-pos4lf-1 );
            }
            break;

        case MG_EV_SEND:
            break;

        case MG_EV_POLL:
            
            if ( conn->flags & MG_F_USER_1) {

                pCtrlObject->getTCPIPServer()->sendOneEventFromQueue( conn, pCtrlObject, false );

                // Send '+OK<CR><LF>' every two seconds to indicate that the
                // link is open
                if ( ( wxGetUTCTime()-pClientItem->m_timeRcvLoop ) > 2 ) {
                    pClientItem->m_timeRcvLoop = wxGetUTCTime();
                    mg_send( conn, "+OK\r\n", 5 );
                }
                
            }
            break;

        default:
            break;
    }
}


///////////////////////////////////////////////////////////////////////////////
// CommandHandler
//

void
VSCPClientThread::CommandHandler( struct mg_connection *conn,
                                    CControlObject *pCtrlObject,
                                    wxString& strCommand )
{
    CClientItem *pClientItem = NULL;
    bool repeat = false;
        if ( NULL == conn ) {
        return;
    }

    pClientItem = ( CClientItem * )conn->user_data;

    if ( NULL == pCtrlObject ) {
        conn->flags |= MG_F_CLOSE_IMMEDIATELY;  // Close connection
        return;
    }

    if ( NULL == pClientItem ) {
        pCtrlObject->logMsg ( _( "[TCP/IP Client] ClientItem pointer is NULL in command handler.\n" )  );
        conn->flags |= MG_F_CLOSE_IMMEDIATELY;  // Close connection
        return;
    }

    pClientItem->m_currentCommand = strCommand;
    pClientItem->m_currentCommandUC = pClientItem->m_currentCommand.Upper();
    pClientItem->m_currentCommand.Trim();
    pClientItem->m_currentCommand.Trim( false );

    // If nothing to handle just return
    if ( 0 == pClientItem->m_currentCommand.Length() ) {
        mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
        return;
    }

    pClientItem->m_currentCommandUC.Trim();
    pClientItem->m_currentCommandUC.Trim( false );
    //wxLogDebug( _("Argument = ") + pClientItem->m_currentCommandUC );

    // If we are in a receive loop only the quitloop command works
    if ( conn->flags & MG_F_USER_1 ) {
        if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "QUITLOOP" ) ) ) {
            conn->flags &= ~(unsigned int)MG_F_USER_1;
            mg_send( conn, MSG_QUIT_LOOP, strlen ( MSG_QUIT_LOOP ) );
            return;
        }
        else {
            return;
        }
    }
     
REPEAT_COMMAND:

    //*********************************************************************
    //                            No Operation
    //*********************************************************************
    if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "NOOP" ) ) ) {
        mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
    }

    //*********************************************************************
    //                             Send event
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SEND " ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 4 ) ) {
            handleClientSend( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                            Read event
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "RETR" ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 2 ) ) {
            handleClientReceive( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                            Data Available
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CDTA" ) ) ) ||
               ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CHKDATA" ) ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 1 ) ) {
            handleClientDataAvailable( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                          Clear input queue
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CLRA" ) ) ) ||
                ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CLRALL" ) ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 1 ) ) {
            handleClientClearInputQueue( conn, pCtrlObject );
        }
    }


    //*********************************************************************
    //                           Get Statistics
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "STAT" ) ) ) {
         if ( checkPrivilege( conn, pCtrlObject, 1 ) ) {
             handleClientGetStatistics( conn, pCtrlObject );
         }
    }

    //*********************************************************************
    //                            Get Status
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "INFO" ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 1 ) ) {
            handleClientGetStatus( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                           Get Channel ID
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CHID" ) ) ) ||
                ( 0 == pClientItem->m_currentCommandUC.Find ( _( "GETCHID" ) ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 1 ) ) {
            handleClientGetChannelID( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                          Set Channel GUID
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SGID" ) ) ) ||
                ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SETGUID" ) ) )) {
        if ( checkPrivilege( conn, pCtrlObject, 6 ) ) {
            handleClientSetChannelGUID( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                          Get Channel GUID
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "GGID" ) ) ) ||
                ( 0 == pClientItem->m_currentCommandUC.Find ( _( "GETGUID" ) ) )  ) {
        if ( checkPrivilege( conn, pCtrlObject, 1 ) ) {
            handleClientGetChannelGUID( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                           Get Version
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "VERS" ) ) ) ||
            ( 0 == pClientItem->m_currentCommandUC.Find ( _( "VERSION" ) ) ) ) {
        handleClientGetVersion( conn, pCtrlObject );
    }

    //*********************************************************************
    //                           Set Filter
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SFLT" ) ) ) ||
               ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SETFILTER" ) ) )  ) {
        if ( checkPrivilege( conn, pCtrlObject, 6 ) ) {
            handleClientSetFilter( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                           Set Mask
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SMSK" ) ) ) ||
                ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SETMASK" ) ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 6 ) ) {
            handleClientSetMask( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                           Username
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "USER " ) ) ) {
        handleClientUser( conn, pCtrlObject );
    }

    //*********************************************************************
    //                            Password
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "PASS " ) ) ) {
        if ( !handleClientPassword( conn, pCtrlObject ) ) {
            pCtrlObject->logMsg ( _( "[TCP/IP Client] Command: Password. Not authorized.\n" ),
                                    DAEMON_LOGMSG_NORMAL,
                                    DAEMON_LOGTYPE_SECURITY );
            conn->flags |= MG_F_CLOSE_IMMEDIATELY;  // Close connection
            return;
        }
    }

    //*********************************************************************
    //                           Challenge
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CHALLENGE " ) ) ) {
        handleChallenge( conn, pCtrlObject );
    }

    //*********************************************************************
    //                        + (repeat last command)
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "+" ) ) ) {
        // Repeat last command
        pClientItem->m_currentCommand = pClientItem->m_lastCommand;
        pClientItem->m_currentCommandUC = pClientItem->m_lastCommand.Upper();
        goto REPEAT_COMMAND;
    }

    //*********************************************************************
    //                             Rcvloop
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "RCVLOOP" ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 2 ) ) {
            pClientItem->m_timeRcvLoop = wxGetUTCTime();
            handleClientRcvLoop( conn, pCtrlObject );
        }
    }


    // *********************************************************************
    //                                 QUIT
    // *********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "QUIT" ) ) ) {
        //long test = MG_F_CLOSE_IMMEDIATELY;
        pCtrlObject->logMsg( _( "[TCP/IP Client] Command: Close.\n" ) );
        mg_send( conn, MSG_GOODBY, strlen ( MSG_GOODBY ) );
        //conn->flags = NSF_FINISHED_SENDING_DATA;    // Close connection
        conn->flags = MG_F_SEND_AND_CLOSE;  // Close connection
        return;
    }

    //*********************************************************************
    //                             Help
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "HELP" ) ) ) {
        handleClientHelp( conn, pCtrlObject );
    }

    //*********************************************************************
    //                             Restart
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "RESTART" ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientRestart( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                             Driver
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "DRIVER " ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientDriver( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                               DM
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "DM " ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientDm( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                             Variable
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "VAR " ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 4 ) ) {
            handleClientVariable( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                               File
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "FILE " ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientFile( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                               UDP
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "UDP " ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientUdp( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                         Client/interface
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "CLIENT " ) ) ) {
        pClientItem->m_currentCommandUC =
            pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-7 ); // Remove "CLIENT "
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientInterface( conn, pCtrlObject );
        }
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "INTERFACE " ) ) ) {
        pClientItem->m_currentCommandUC =
            pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-10 ); // Remove "INTERFACE "
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientInterface( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                               User
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "USER " ) ) ) {
        handleClientUser( conn, pCtrlObject );
    }

    //*********************************************************************
    //                               Test
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "TEST" ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientTest( conn, pCtrlObject );
        }
    }


    //*********************************************************************
    //                              Shutdown
    //*********************************************************************
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SHUTDOWN" ) ) ) {
        if ( checkPrivilege( conn, pCtrlObject, 15 ) ) {
            handleClientShutdown( conn, pCtrlObject );
        }
    }

    //*********************************************************************
    //                             WhatCanYouDo
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "WHATCANYOUDO" ) ) ) ||
               ( 0 == pClientItem->m_currentCommandUC.Find ( _( "WCYD" ) ) ) ) {
        handleClientCapabilityRequest( conn, pCtrlObject );
    }

    //*********************************************************************
    //                             Measurement
    //*********************************************************************
    else if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "MEASUREMENT" ) ) ) ) {
        handleClientMeasurment( conn, pCtrlObject );
    }

    //*********************************************************************
    //                                What?
    //*********************************************************************
    else {
        mg_send( conn,  MSG_UNKNOWN_COMMAND, strlen ( MSG_UNKNOWN_COMMAND ) );
    }

    pClientItem->m_lastCommand = pClientItem->m_currentCommand;

}


///////////////////////////////////////////////////////////////////////////////
// handleClientMeasurment
//
// format,level,vscp-measurement-type,value,unit,guid,sensoridx,zone,subzone,dest-guid
//
// format                   float|string|0|1 - float=0, string=1.
// level                    1|0  1 = VSCP Level I event, 2 = VSCP Level II event.
// vscp-measurement-type    A valid vscp measurement type.
// value                    A floating point value. (use $ prefix for variable followed by name)
// unit                     Optional unit for this type. Default = 0.
// guid                     Optional GUID (or "-"). Default is "-".
// sensoridx                Optional sensor index. Default is 0.
// zone                     Optional zone. Default is 0.
// subzone                  Optional subzone- Default is 0.
// dest-guid                Optional destination GUID. For Level I over Level II.
//

void VSCPClientThread::handleClientMeasurment( struct mg_connection *conn,
                                                 CControlObject *pCtrlObject )
{
    wxString wxstr;
    unsigned long l;
    double value = 0;
    cguid guid;             // Initialized to zero
    cguid destguid;         // Initialized to zero
    long level = VSCP_LEVEL2;
    long unit = 0;
    long vscptype;
    long sensoridx = 0;
    long zone = 0;
    long subzone = 0;
    long eventFormat = 0;   // float
    uint8_t data[ VSCP_MAX_DATA ];
    uint16_t sizeData;
    
    // Check objects
    if ( ( NULL == conn ) || (NULL == pCtrlObject )  ) {
        mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
        return;
    }
    
    CClientItem *pClientItem = (CClientItem *)conn->user_data;
    if ( NULL == pClientItem ) {
        mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
        return;
    }
    
    wxStringTokenizer tkz( pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 11 ),
                            _(",") );

    // If first character is $ user request us to send content from
    // a variable

    // * * * event format * * *
    
    // Get event format (float | string | 0 | 1 - float=0, string=1.)
    if ( !tkz.HasMoreTokens() ) {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    wxstr = tkz.GetNextToken();
    wxstr.MakeUpper();
    if ( wxstr.IsNumber() ) {
        
        l = 0;
  
        if ( wxstr.ToULong( &l ) && ( l > 1 ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }
        
        eventFormat = l;
        
    }
    else {
        if ( wxNOT_FOUND != wxstr.Find("STRING") ) {
            eventFormat = 1;
        }
        else if ( wxNOT_FOUND != wxstr.Find("FLOAT") ) {
            eventFormat = 0;
        }
        else {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }
    }
    
    // * * * Level * * *
    
    if ( !tkz.HasMoreTokens() ) {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    wxstr = tkz.GetNextToken();
    
    if ( wxstr.IsNumber() ) {
        
        l = VSCP_LEVEL1; 
  
        if ( wxstr.ToULong( &l ) && ( l > VSCP_LEVEL2 ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }
        
        level = l;
        
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    // * * * vscp-measurement-type * * *
    
    if ( !tkz.HasMoreTokens() ) {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    wxstr = tkz.GetNextToken();
    
    if ( wxstr.IsNumber() ) {
        
        l = 0; 
  
        if ( wxstr.ToULong( &l ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }
        
        level = l;
        
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }
    

    // * * * value * * *
    
        
    if ( !tkz.HasMoreTokens() ) {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    wxstr = tkz.GetNextToken();
    wxstr.Trim();
    wxstr.Trim(false);
    
    // If first character is '$' user request us to send content from
    // a variable (that must be numeric)
    if ( '$' == wxstr[0] ) {
        
        CVSCPVariable variable;
        wxstr = wxstr.Right( wxstr.Length() - 1 );  // get variable name

        wxstr.MakeUpper();

        if ( m_pCtrlObject->m_VSCP_Variables.find( wxstr, variable  ) ) {
            mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
            return;
        }
        
        // get the value
        wxstr = variable.getValue();
        if ( !wxstr.IsNumber() ) {
            mg_send( conn, MSG_VARIABLE_NOT_NUMERIC, strlen ( MSG_VARIABLE_NOT_NUMERIC ) );
            return;
        }
        
        if ( wxstr.ToCDouble( &value ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }
        
    }
    else {
        if ( !wxstr.ToCDouble( &value ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }
    }
    
    
    // * * * unit * * *
    
    
    if ( tkz.HasMoreTokens() ) {
        wxstr = tkz.GetNextToken();
        if ( wxstr.IsNumber() ) {
            if ( wxstr.ToULong( &l ) ) {
                unit = l;
            }
        }
    }
    
    
    // * * * guid * * *
    
       
    if ( tkz.HasMoreTokens() ) {
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        // If empty set to default.
        if ( 0 == wxstr.Length() ) wxstr = _("-");
        guid.getFromString( wxstr );
    }
    
    
    // * * * sensor index * * *
    
    
    if ( tkz.HasMoreTokens() ) {
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        
        if ( wxstr.IsNumber() ) { 
            if ( wxstr.ToULong( &l ) ) {
                sensoridx = l;
            }
        }
    }
    
    
    // * * * zone * * *
    
    
    if ( tkz.HasMoreTokens() ) {
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        
        if ( wxstr.IsNumber() ) { 
            if ( wxstr.ToULong( &l ) ) {
                zone = l;
                zone &= 0xff;
            }
        }
    }
    
    
    // * * * subzone * * *
    
    
    if ( tkz.HasMoreTokens() ) {
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        
        if ( wxstr.IsNumber() ) { 
            if ( wxstr.ToULong( &l ) ) {
                subzone = l;
                subzone &= 0xff;
            }
        }
    }
    
    
    // * * * destguid * * *
    
        
    if ( tkz.HasMoreTokens() ) {
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        // If empty set to default.
        if ( 0 == wxstr.Length() ) wxstr = _("-");
        destguid.getFromString( wxstr );
    }
    
    // Range checks
    if ( VSCP_LEVEL1 == level ) {
        if (unit > 3) unit = 0;
        if (sensoridx > 7) unit = 0;
        if (vscptype > 512) vscptype -= 512;
    } 
    else {  // VSCP_LEVEL2
        if (unit > 255) unit &= 0xff;
        if (sensoridx > 255) sensoridx &= 0xff;
    }
    
    if ( 1 == level ) {

        if ( 0 == eventFormat ) {

            // * * * Floating point * * *
            
            if ( vscp_convertFloatToFloatEventData( data,
                                                        &sizeData,
                                                        value,
                                                        unit,
                                                        sensoridx ) ) {
                if ( sizeData > 8 ) sizeData = 8;

                vscpEvent *pEvent = new vscpEvent;
                if ( NULL == pEvent ) {
                    mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
                    return;
                }
                
                pEvent->pdata = NULL;
                pEvent->head = VSCP_PRIORITY_NORMAL;
                pEvent->timestamp = 0;  // Let interface fill in
                guid.writeGUID( pEvent->GUID );
                pEvent->sizeData = sizeData;
                if ( sizeData > 0 ) {
                    pEvent->pdata = new uint8_t[ sizeData ];
                    memcpy( pEvent->pdata, data, sizeData );
                }
                pEvent->vscp_class = VSCP_CLASS1_MEASUREMENT;
                pEvent->vscp_type = vscptype;

                // send the event
                if ( !pCtrlObject->sendEvent( pClientItem, pEvent ) ) {
                    mg_send( conn,  MSG_UNABLE_TO_SEND_EVENT, strlen ( MSG_UNABLE_TO_SEND_EVENT ) );
                    return;
                }

            } 
            else {
                mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            }
        } 
        else {
            
            // * * * String * * *
            
            vscpEvent *pEvent = new vscpEvent;
            if (NULL == pEvent) {
                mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
                return;
            }
            
            pEvent->pdata = NULL;

            if ( !vscp_makeStringMeasurementEvent( pEvent,
                                                    value,
                                                    unit,
                                                    sensoridx ) ) {
                mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            }
            
        }
    } 
    else {      // Level II
        
        if ( 0 == eventFormat ) {   // float and Level II

            // * * * Floating point * * *
            
            vscpEvent *pEvent = new vscpEvent;
            if ( NULL == pEvent ) {
                mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
                return;
            }

            pEvent->pdata = NULL;

            pEvent->obid = 0;
            pEvent->head = VSCP_PRIORITY_NORMAL;
            pEvent->timestamp = 0; // Let interface fill in timestamp
            guid.writeGUID( pEvent->GUID );
            pEvent->head = 0;
            pEvent->vscp_class = VSCP_CLASS2_MEASUREMENT_FLOAT;
            pEvent->vscp_type = vscptype;
            pEvent->timestamp = 0;
            pEvent->sizeData = 12;

            data[ 0 ] = sensoridx;
            data[ 1 ] = zone;
            data[ 2 ] = subzone;
            data[ 3 ] = unit;

            memcpy(data + 4, (uint8_t *) & value, 8); // copy in double
            uint64_t temp = wxUINT64_SWAP_ON_LE(*(data + 4));
            memcpy(data + 4, (void *) &temp, 8);

            // Copy in data
            pEvent->pdata = new uint8_t[ 4 + 8 ];
            if (NULL == pEvent->pdata) {
                mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
                delete pEvent;
                return;
            }
            
            memcpy(pEvent->pdata, data, 4 + 8);

            // send the event
            if ( !pCtrlObject->sendEvent( pClientItem, pEvent ) ) {
                mg_send( conn,  MSG_UNABLE_TO_SEND_EVENT, strlen ( MSG_UNABLE_TO_SEND_EVENT ) );
                return;
            }
            
        } 
        else {      // string & Level II
            
            // * * * String * * *
            
            vscpEvent *pEvent = new vscpEvent;
            pEvent->pdata = NULL;

            pEvent->obid = 0;
            pEvent->head = VSCP_PRIORITY_NORMAL;
            pEvent->timestamp = 0; // Let interface fill in
            guid.writeGUID( pEvent->GUID );
            pEvent->head = 0;
            pEvent->vscp_class = VSCP_CLASS2_MEASUREMENT_STR;
            pEvent->vscp_type = vscptype;
            pEvent->timestamp = 0;
            pEvent->sizeData = 12;
            
            wxString strValue = wxString::Format(_("%f"), value );

            data[ 0 ] = sensoridx;
            data[ 1 ] = zone;
            data[ 2 ] = subzone;
            data[ 3 ] = unit;

            pEvent->pdata = new uint8_t[ 4 + strValue.Length() ];
            if (NULL == pEvent->pdata) {
                mg_send( conn,  MSG_INTERNAL_MEMORY_ERROR, strlen ( MSG_INTERNAL_MEMORY_ERROR ) );
                delete pEvent;
                return;
            }
            memcpy(data + 4, strValue.mbc_str(), strValue.Length()); // copy in double

            // send the event
            if ( !pCtrlObject->sendEvent( pClientItem, pEvent ) ) {
                mg_send( conn,  MSG_UNABLE_TO_SEND_EVENT, strlen ( MSG_UNABLE_TO_SEND_EVENT ) );
                return;
            }
            
        }
    }
   
    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleClientCapabilityRequest
//

void VSCPClientThread::handleClientCapabilityRequest( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    wxString wxstr;
    uint8_t capabilities[16];
    
    gpobj->getVscpCapabilities( capabilities );
    wxstr = wxString::Format(_("%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X"),                                
                                capabilities[7], capabilities[6], capabilities[5], capabilities[4],
                                capabilities[3], capabilities[2], capabilities[1], capabilities[0] );
    mg_send( conn,  MSG_UNKNOWN_COMMAND, strlen ( MSG_UNKNOWN_COMMAND ) );
}


///////////////////////////////////////////////////////////////////////////////
// isVerified
//

bool VSCPClientThread::isVerified( struct mg_connection *conn,
                                        CControlObject *pCtrlObject )
{
    // Check objects
    if ( ( NULL == conn ) || (NULL == pCtrlObject )  ) {
        return false;
    }
    
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn, MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// checkPrivilege
//

bool VSCPClientThread::checkPrivilege( struct mg_connection *conn,
                                            CControlObject *pCtrlObject,
                                            unsigned char reqiredPrivilege )
{
    // Check objects
    if ( ( NULL == conn ) || (NULL == pCtrlObject )  ) {
        return false;
    }
    
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be loged on
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return false;
    }

    if ( NULL == pClientItem->m_pUserItem ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return false;
    }

    if ( (pClientItem->m_pUserItem->getUserRights( 0 ) & USER_PRIVILEGE_MASK ) < reqiredPrivilege ) {
        mg_send( conn,  MSG_LOW_PRIVILEGE_ERROR, strlen ( MSG_LOW_PRIVILEGE_ERROR ) );
        return false;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// handleClientSend
//

void VSCPClientThread::handleClientSend( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    bool bSent = false;
    bool bVariable = false;
    vscpEvent event;
    wxString nameVariable;
    CClientItem *pClientItem = NULL;
    if ( NULL == conn ) {
        return;
    }

    if ( NULL == pCtrlObject ) {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    pClientItem = ( CClientItem * )conn->user_data;

    if ( NULL == pClientItem ) {
        mg_send( conn, MSG_INTERNAL_ERROR, strlen( MSG_INTERNAL_ERROR ) );
        return;
    }

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn, MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    wxString str = pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 5 );
    wxStringTokenizer tkz( str, _(",") );

    // If first character is $ user request us to send content from
    // a variable

    // Get head
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        str.Trim( false );
        if ( wxNOT_FOUND == str.Find(_("$") ) ) {
            event.head = vscp_readStringValue( str );
        }
        else {
            CVSCPVariable variable;
            bVariable = true;   // Yes this is a variable send

            // Get the name of the variable
            nameVariable = str.Right( str.Length() - 1 );
            nameVariable.MakeUpper();

            if ( m_pCtrlObject->m_VSCP_Variables.find( nameVariable, variable  ) ) {
                mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
                return;
            }

            // Must be event type
            if ( VSCP_DAEMON_VARIABLE_CODE_VSCP_EVENT != variable.getType() ) {
                mg_send( conn, MSG_VARIABLE_MUST_BE_EVENT_TYPE, strlen ( MSG_VARIABLE_MUST_BE_EVENT_TYPE ) );
                return;
            }

            // Get the event
            variable.getValue( &event );

        }
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    if ( !bVariable ) {

        // Get Class
        if ( tkz.HasMoreTokens() ) {
            str = tkz.GetNextToken();
            event.vscp_class = vscp_readStringValue( str );
        }
        else {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }

        // Get Type
        if ( tkz.HasMoreTokens() ) {
            str = tkz.GetNextToken();
            event.vscp_type = vscp_readStringValue( str );
        }
        else {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }

        // Get OBID  -  Kept here to be compatible with receive
        if ( tkz.HasMoreTokens() ) {
            str = tkz.GetNextToken();
            event.obid = vscp_readStringValue( str );
        }
        else {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }

        // Get Timestamp
        if ( tkz.HasMoreTokens() ) {
            str = tkz.GetNextToken();
            event.timestamp = vscp_readStringValue( str );
            if ( !event.timestamp ) {
                event.timestamp = vscp_makeTimeStamp();
            }
        }
        else {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }

        // Get GUID
        wxString strGUID;
        if ( tkz.HasMoreTokens() ) {
            strGUID = tkz.GetNextToken();

            // Check if i/f GUID should be used
            if ( ( '-' == strGUID[0] ) || vscp_isGUIDEmpty( event.GUID ) ) {
                // Copy in the i/f GUID
                pClientItem->m_guid.writeGUID( event.GUID );
            }
            else {
                vscp_getGuidFromString( &event, strGUID );
            }

        }
        else {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            return;
        }

        // Handle data
        if ( 512 < tkz.CountTokens() ) {
            mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
            return;
        }

        event.sizeData = tkz.CountTokens();

        if ( event.sizeData > 0 ) {

            unsigned int index = 0;

            event.pdata = new uint8_t[ event.sizeData ];

            if ( NULL == event.pdata ) {
                mg_send( conn, MSG_INTERNAL_MEMORY_ERROR, strlen( MSG_INTERNAL_MEMORY_ERROR ) );
                return;
            }

            while ( tkz.HasMoreTokens() && ( event.sizeData > index ) ) {
                str = tkz.GetNextToken();
                event.pdata[ index++ ] = vscp_readStringValue( str );
            }

            if ( tkz.HasMoreTokens() ) {
                mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
                return;
            }
        }
        else {
            // No data
            event.pdata = NULL;
        }

    } // not variable send

    // Check if this user is allowed to send this event
    if ( !pClientItem->m_pUserItem->isUserAllowedToSendEvent( event.vscp_class, event.vscp_type ) ) {
        wxString strErr =
                        wxString::Format( _("[tcp/ip Client] User [%s] not allowed to send event class=%d type=%d.\n"),
                                                (const char *)pClientItem->m_pUserItem->getUser().mbc_str(),
                                                event.vscp_class, event.vscp_type );

        pCtrlObject->logMsg ( strErr, DAEMON_LOGMSG_NORMAL, DAEMON_LOGTYPE_SECURITY );

        mg_send( conn, MSG_MOT_ALLOWED_TO_SEND_EVENT, strlen ( MSG_MOT_ALLOWED_TO_SEND_EVENT ) );

        if ( NULL != event.pdata ) {
            delete [] event.pdata;
            event.pdata = NULL;
        }

        return;
    }
    
    // send event
    if ( !m_pCtrlObject->sendEvent( pClientItem, &event ) ) {
        mg_send( conn,  MSG_BUFFER_FULL, strlen ( MSG_BUFFER_FULL ) );
        return;
    }
    
/*
    vscpEvent *pEvent = new vscpEvent;		// Create new VSCP Event
    if ( NULL != pEvent ) {

        // Copy event
        vscp_copyVSCPEvent( pEvent, &event );

        // We don't need the original event anymore
        if ( NULL != event.pdata ) {
            delete [] event.pdata;
            event.pdata = NULL;
            event.sizeData = 0;
        }

        // Save the originating clients id so
        // this client don't get the message back
        pEvent->obid = pClientItem->m_clientID;


        // Level II events between 512-1023 is recognised by the daemon and
        // sent to the correct interface as Level I events if the interface
        // is addressed by the client.
        if (( pEvent->vscp_class <= 1023 ) &&
            ( pEvent->vscp_class >= 512 ) &&
            ( pEvent->sizeData >= 16 )	) {

            // This event should be sent to the correct interface if it is
            // available on this machine. If not it should be sent to
            // the rest of the network as normal

            cguid destguid;
            destguid.getFromArray( pEvent->pdata );

            destguid.setAt(0,0);    // Interface GUID's have LSB bytes nilled
            destguid.setAt(1,0);

            wxString dbgStr =
                    wxString::Format( _("Level I event over Level II dest = %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:"),
                    destguid.getAt(15),destguid.getAt(14),destguid.getAt(13),destguid.getAt(12),
                    destguid.getAt(11),destguid.getAt(10),destguid.getAt(9),destguid.getAt(8),
                    destguid.getAt(7),destguid.getAt(6),destguid.getAt(5),destguid.getAt(4),
                    destguid.getAt(3),destguid.getAt(2),destguid.getAt(1),destguid.getAt(0) );
                    m_pCtrlObject->logMsg( dbgStr, DAEMON_LOGMSG_DEBUG );

            // Find client
            m_pCtrlObject->m_wxClientMutex.Lock();

            CClientItem *pDestClientItem = NULL;
            VSCPCLIENTLIST::iterator iter;
            for (iter = m_pCtrlObject->m_clientList.m_clientItemList.begin();
                    iter != m_pCtrlObject->m_clientList.m_clientItemList.end();
                    ++iter) {

                CClientItem *pItem = *iter;
                dbgStr =
                    wxString::Format( _("Test if = %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:"),
                    pItem->m_guid.getAt(15),pItem->m_guid.getAt(14),pItem->m_guid.getAt(13),pItem->m_guid.getAt(12),
                    pItem->m_guid.getAt(11),pItem->m_guid.getAt(10),pItem->m_guid.getAt(9),pItem->m_guid.getAt(8),
                    pItem->m_guid.getAt(7),pItem->m_guid.getAt(6),pItem->m_guid.getAt(5),pItem->m_guid.getAt(4),
                    pItem->m_guid.getAt(3),pItem->m_guid.getAt(2),pItem->m_guid.getAt(1),pItem->m_guid.getAt(0) );
                    dbgStr += _(" ");
                    dbgStr += pItem->m_strDeviceName;
                    m_pCtrlObject->logMsg( dbgStr, DAEMON_LOGMSG_DEBUG );

                    if ( pItem->m_guid == destguid ) {
                        // Found
                        pDestClientItem = pItem;
                        bSent = true;
                        dbgStr = _("Match ");
                        m_pCtrlObject->logMsg( dbgStr, DAEMON_LOGMSG_DEBUG );
                        m_pCtrlObject->sendEventToClient( pItem, pEvent );
                        break;
                    }

                }

                m_pCtrlObject->m_wxClientMutex.Unlock();

        }


        if ( !bSent ) {

            // There must be room in the send queue
            if ( m_pCtrlObject->m_maxItemsInClientReceiveQueue >
                m_pCtrlObject->m_clientOutputQueue.GetCount() ) {

                    m_pCtrlObject->m_mutexClientOutputQueue.Lock();
                    m_pCtrlObject->m_clientOutputQueue.Append ( pEvent );
                    m_pCtrlObject->m_semClientOutputQueue.Post();
                    m_pCtrlObject->m_mutexClientOutputQueue.Unlock();

                    // TX Statistics
                    pClientItem->m_statistics.cntTransmitData += pEvent->sizeData;
                    pClientItem->m_statistics.cntTransmitFrames++;

            }
            else {

                pClientItem->m_statistics.cntOverruns++;

                vscp_deleteVSCPevent( pEvent );
                mg_send( conn,  MSG_BUFFER_FULL, strlen ( MSG_BUFFER_FULL ) );
            }

        }

    } // Valid pEvent
*/    

   mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleClientReceive
//

void VSCPClientThread::handleClientReceive ( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    unsigned short cnt = 0;	// # of messages to read
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    wxString str = pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 4 );
    cnt = vscp_readStringValue( str );

    if ( !cnt ) cnt = 1;	// No arg is "read one"


    // Read cnt messages
    while ( cnt ) {

        wxString strOut;

        if ( !pClientItem->m_bOpen ) {
            mg_send( conn,  MSG_NO_MSG, strlen ( MSG_NO_MSG ) );
            return;
        }
        else {
            if ( false == sendOneEventFromQueue( conn, pCtrlObject ) ) {
                return;
            }
        }

        cnt--;

    } // while

    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );

}

///////////////////////////////////////////////////////////////////////////////
// sendOneEventFromQueue
//

bool VSCPClientThread::sendOneEventFromQueue( struct mg_connection *conn,
                                                CControlObject *pCtrlObject,
                                                bool bStatusMsg )
{
    wxString strOut;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    CLIENTEVENTLIST::compatibility_iterator nodeClient;

    if ( pClientItem->m_clientInputQueue.GetCount() ) {

        vscpEvent *pqueueEvent;
        pClientItem->m_mutexClientInputQueue.Lock();
        {
            nodeClient = pClientItem->m_clientInputQueue.GetFirst();
            pqueueEvent = nodeClient->GetData();

            // Remove the node
            pClientItem->m_clientInputQueue.DeleteNode ( nodeClient );
        }
        pClientItem->m_mutexClientInputQueue.Unlock();

        strOut.Printf( _("%hu,%hu,%hu,%lu,%lu,"),
                            (unsigned short)pqueueEvent->head,
                            (unsigned short)pqueueEvent->vscp_class,
                            (unsigned short)pqueueEvent->vscp_type,
                            (unsigned long)pqueueEvent->obid,
                            (unsigned long)pqueueEvent->timestamp );

        wxString strGUID;
        vscp_writeGuidToString( pqueueEvent, strGUID );
        strOut += strGUID;

        // Handle data
        if ( NULL != pqueueEvent->pdata ) {

            strOut += _(",");
            for ( int i=0; i<pqueueEvent->sizeData; i++ ) {
                wxString wrk;
                wrk.Printf(_("%d"), pqueueEvent->pdata[ i ] );
                if ( ( pqueueEvent->sizeData - 1 ) != i ) {
                    wrk += _(",");
                }

                strOut += wrk;

            }

        }

        strOut += _("\r\n");
        mg_send( conn,  strOut.mb_str(), strlen ( strOut.mb_str() ) );

        //delete pqueueEvent;
        vscp_deleteVSCPevent( pqueueEvent );

        // Let the system work a little
        //ns_mgr_poll( &m_pCtrlObject->m_mgrTcpIpServer, 1 );
    }
    else {
        if ( bStatusMsg ) {
            mg_send( conn,  MSG_NO_MSG, strlen ( MSG_NO_MSG ) );
        }

        return false;

    }

    return true;

}



///////////////////////////////////////////////////////////////////////////////
// handleClientDataAvailable
//

void VSCPClientThread::handleClientDataAvailable ( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    char outbuf[ 1024 ];
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED,
            strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    sprintf ( outbuf,
        "%zd\r\n%s",
        pClientItem->m_clientInputQueue.GetCount(),
        MSG_OK );
    mg_send( conn,  outbuf, strlen ( outbuf ) );


}

///////////////////////////////////////////////////////////////////////////////
// handleClientClearInputQueue
//

void VSCPClientThread::handleClientClearInputQueue ( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED,
            strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    pClientItem->m_mutexClientInputQueue.Lock();
    pClientItem->m_clientInputQueue.Clear();
    pClientItem->m_mutexClientInputQueue.Unlock();

    mg_send( conn,  MSG_QUEUE_CLEARED, strlen ( MSG_QUEUE_CLEARED ) );
}


///////////////////////////////////////////////////////////////////////////////
// handleClientGetStatistics
//

void VSCPClientThread::handleClientGetStatistics ( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    char outbuf[ 1024 ];
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    sprintf ( outbuf, "%lu,%lu,%lu,%lu,%lu,%lu,%lu\r\n%s",
        pClientItem->m_statistics.cntBusOff,
        pClientItem->m_statistics.cntBusWarnings,
        pClientItem->m_statistics.cntOverruns,
        pClientItem->m_statistics.cntReceiveData,
        pClientItem->m_statistics.cntReceiveFrames,
        pClientItem->m_statistics.cntTransmitData,
        pClientItem->m_statistics.cntTransmitFrames,
        MSG_OK );


    mg_send( conn, outbuf, strlen ( outbuf ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientGetStatus
//

void VSCPClientThread::handleClientGetStatus ( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    char outbuf[ 1024 ];
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    sprintf ( outbuf, "%lu,%lu,%lu,\"%s\"\r\n%s",
        pClientItem->m_status.channel_status,
        pClientItem->m_status.lasterrorcode,
        pClientItem->m_status.lasterrorsubcode,
        pClientItem->m_status.lasterrorstr,
        MSG_OK );

    mg_send( conn, outbuf, strlen ( outbuf ) );


}

///////////////////////////////////////////////////////////////////////////////
// handleClientGetChannelID
//

void VSCPClientThread::handleClientGetChannelID ( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    char outbuf[ 1024 ];
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    sprintf ( outbuf, "%lu\r\n%s",
        (unsigned long)pClientItem->m_clientID, MSG_OK );

    mg_send( conn,  outbuf, strlen ( outbuf ) );

}


///////////////////////////////////////////////////////////////////////////////
// handleClientSetChannelGUID
//

void VSCPClientThread::handleClientSetChannelGUID ( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    wxString str;
    if ( ( 0 == pClientItem->m_currentCommandUC.Find ( _( "SGID" ) ) ) ) {
        str = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length() - 5 ); // remove: command + space
    }
    else {
        str = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length() - 8 ); // remove: command + space
    }

    pClientItem->m_guid.getFromString(str);

    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleClientGetChannelGUID
//

void VSCPClientThread::handleClientGetChannelGUID ( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    wxString strBuf;
    //char outbuf[ 1024 ];
    //char wrkbuf[ 20 ];
    //int i;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn, MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }


    pClientItem->m_guid.toString( strBuf );
    strBuf += _("\r\n");
    strBuf += _(MSG_OK);

    mg_send( conn, strBuf.mb_str(), strlen( strBuf.mb_str() ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientGetVersion
//

void VSCPClientThread::handleClientGetVersion ( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    char outbuf[ 1024 ];

    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }


    sprintf ( outbuf,
                "%d,%d,%d\r\n%s",
                VSCPD_MAJOR_VERSION,
                VSCPD_MINOR_VERSION,
                VSCPD_RELEASE_VERSION,
                MSG_OK );

    mg_send( conn,  outbuf, strlen ( outbuf ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientSetFilter
//

void VSCPClientThread::handleClientSetFilter ( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    wxString str;
    if (  0 == pClientItem->m_currentCommandUC.Find ( _( "SFLT" ) ) ) {
        str = pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 5 );
    }
    else {
        str = pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 10 );  // SETFILTER
    }
    wxStringTokenizer tkz( str, _(",") );

    // Get priority
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        pClientItem->m_filterVSCP.filter_priority = vscp_readStringValue( str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    // Get Class
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        pClientItem->m_filterVSCP.filter_class = vscp_readStringValue( str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }


    // Get Type
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        pClientItem->m_filterVSCP.filter_type = vscp_readStringValue( str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    // Get GUID
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        vscp_getGuidFromStringToArray( pClientItem->m_filterVSCP.filter_GUID, str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientSetMask
//

void VSCPClientThread::handleClientSetMask ( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Must be accredited to do this
    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_NOT_ACCREDITED, strlen ( MSG_NOT_ACCREDITED ) );
        return;
    }

    wxString str;
    if (  0 == pClientItem->m_currentCommandUC.Find ( _( "SMSK" ) ) ) {
        str = pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 5 );
    }
    else {
        str = pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 8 );  // SETMASK
    }
    wxStringTokenizer tkz( str, _(",") );

    // Get priority
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        pClientItem->m_filterVSCP.mask_priority = vscp_readStringValue( str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    // Get Class
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        pClientItem->m_filterVSCP.mask_class = vscp_readStringValue( str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }


    // Get Type
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        pClientItem->m_filterVSCP.mask_type = vscp_readStringValue( str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    // Get GUID
    if ( tkz.HasMoreTokens() ) {
        str = tkz.GetNextToken();
        vscp_getGuidFromStringToArray( pClientItem->m_filterVSCP.mask_GUID, str );
    }
    else {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientUser
//

void VSCPClientThread::handleClientUser ( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
        return;
    }

    pClientItem->m_UserName =
        pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 4 );
    pClientItem->m_UserName.Trim();         // Trim right side
    pClientItem->m_UserName.Trim( false );  // Trim left
    if ( pClientItem->m_UserName.IsEmpty() ) {
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
        return;
    }

    mg_send( conn,  MSG_USENAME_OK, strlen ( MSG_USENAME_OK ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientPassword
//

bool VSCPClientThread::handleClientPassword ( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
        return true;
    }

    // Must have username before password can be entered.
    if ( 0 == pClientItem->m_UserName.Length() ) {
        mg_send( conn,  MSG_NEED_USERNAME, strlen ( MSG_NEED_USERNAME ) );
        return true;
    }

    wxString strPassword =
            pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length() - 4 );
    strPassword.Trim();             // Trim right side
    strPassword.Trim( false );      // Trim left
    if ( strPassword.IsEmpty() ) {
        pClientItem->m_UserName = _("");
        mg_send( conn,  MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return false;
    }

    /*
    // Calculate MD5 for username:authdomain:password
    char buf[2148];
    memset( buf, 0, sizeof( buf ) );
    strncpy( buf,
                (const char *)pClientItem->m_UserName.mbc_str(),
                pClientItem->m_UserName.Length() );
    strncat( buf, ":", 1 );
    strncat( buf,
                (const char *)pCtrlObject->m_authDomain,
                strlen( pCtrlObject->m_authDomain ) );
    strncat( buf, ":", 1 );
    strncat( (char *)buf, strPassword.mbc_str(), strPassword.Length() );

    MD5_CTX ctx;
    MD5_Init( &ctx );
    MD5_Update( &ctx, (const unsigned char *)buf, strlen( buf ) );
    unsigned char bindigest[16];
    MD5_Final( bindigest, &ctx );
    char digest[33];
    memset( digest, 0, sizeof( digest ) );
    cs_to_hex( digest, bindigest, 16 );

    wxString md5Password = wxString( digest, wxConvUTF8 );
    */
    
    m_pCtrlObject->m_mutexUserList.Lock();
#if  0
    ::wxLogDebug( _("Username: ") + m_UserName );
    ::wxLogDebug( _("Password: ") + strPassword );
    ::wxLogDebug( _("MD5 of Password: ") + md5Password );
#endif
    pClientItem->m_pUserItem = m_pCtrlObject->m_userList.validateUser( pClientItem->m_UserName, strPassword );
    m_pCtrlObject->m_mutexUserList.Unlock();

    if ( NULL == pClientItem->m_pUserItem ) {

        wxString strErr =
            wxString::Format(_("[TCP/IP Client] User [%s][%s] not allowed to connect.\n"),
            (const char *)pClientItem->m_UserName.mbc_str(), (const char *)strPassword.mbc_str() );

        pCtrlObject->logMsg ( strErr, DAEMON_LOGMSG_NORMAL, DAEMON_LOGTYPE_SECURITY );
        mg_send( conn,  MSG_PASSWORD_ERROR, strlen ( MSG_PASSWORD_ERROR ) );
        return false;
    }

    // Get remote address
    struct sockaddr_in cli_addr;
    socklen_t clilen = 0;
    clilen = sizeof (cli_addr);
    (void)getpeername( conn->sock, (struct sockaddr *)&cli_addr, &clilen);
    wxString remoteaddr = wxString::FromAscii( inet_ntoa( cli_addr.sin_addr ) );

    // Check if this user is allowed to connect from this location
    m_pCtrlObject->m_mutexUserList.Lock();
    bool bValidHost =
            pClientItem->m_pUserItem->isAllowedToConnect( remoteaddr );
    m_pCtrlObject->m_mutexUserList.Unlock();

    if ( !bValidHost ) {
        wxString strErr = wxString::Format(_("[TCP/IP Client] Host [%s] not allowed to connect.\n"),
            (const char *)remoteaddr.c_str() );

        pCtrlObject->logMsg ( strErr, DAEMON_LOGMSG_NORMAL, DAEMON_LOGTYPE_SECURITY );
        mg_send( conn,  MSG_INVALID_REMOTE_ERROR, strlen ( MSG_INVALID_REMOTE_ERROR ) );
        return false;
    }

    // Copy in the user filter
    memcpy( &pClientItem->m_filterVSCP,
                pClientItem->m_pUserItem->getFilter(),
                sizeof( vscpEventFilter ) );

    wxString strErr =
        wxString::Format( _("[TCP/IP Client] Host [%s] User [%s] allowed to connect.\n"),
                            (const char *)remoteaddr.c_str(),
                            (const char *)pClientItem->m_UserName.c_str() );

    pCtrlObject->logMsg ( strErr, DAEMON_LOGMSG_NORMAL, DAEMON_LOGTYPE_SECURITY );

    pClientItem->bAuthenticated = true;
    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );

    return true;

}

///////////////////////////////////////////////////////////////////////////////
// handleChallenge
//

void VSCPClientThread::handleChallenge( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    wxString wxstr;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    wxString strcmd = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-4 );
    strcmd.Trim();
    strcmd.Trim(false);

    memset( pClientItem->m_sid, 0, sizeof( pClientItem->m_sid ) );
    if ( !gpobj->generateSessionId( (const char *)strcmd.mbc_str(), pClientItem->m_sid ) ) {
        mg_send( conn,  MSG_FAILD_TO_GENERATE_SID, strlen ( MSG_FAILD_TO_GENERATE_SID ) );
        return; 
    }
    
    wxstr = _("+OK - ") + wxString::FromUTF8( pClientItem->m_sid ) + _("\r\n");
    mg_send( conn, (const char *)wxstr.mbc_str(), strlen ( (const char *)wxstr.mbc_str() ) );
    
    //mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
}


///////////////////////////////////////////////////////////////////////////////
// handleClientRcvLoop
//

void VSCPClientThread::handleClientRcvLoop( struct mg_connection *conn,
                                                CControlObject *pCtrlObject  )
{

    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    mg_send( conn,  MSG_RECEIVE_LOOP, strlen ( MSG_RECEIVE_LOOP ) );
    conn->flags |= MG_F_USER_1; // Mark socket as being in receive loop
    
    pClientItem->m_readBuffer.Empty();

    // Loop until the connection is lost
    /*
    while ( !TestDestroy() && !m_bQuit && (conn->flags & MG_F_USER_1 ) ) {

        // Wait for event
        if ( wxSEMA_TIMEOUT ==
            pClientItem->m_semClientInputQueue.WaitTimeout( 1000 ) ) {
                mg_send( conn, "+OK\r\n", 5 );
                continue;
        }
        
        // We must handle the polling here while in the loop
        mg_mgr_poll( &m_pCtrlObject->m_mgrTcpIpServer, 50 );


    } // While 
    */
    
    return;
}

///////////////////////////////////////////////////////////////////////////////
// handleClientHelp
//

void VSCPClientThread::handleClientHelp( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    wxString strcmd = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-4 );
    strcmd.Trim();
    strcmd.Trim(false);

    if ( _("") == strcmd ) {

        wxString str = _("Help for the VSCP tcp/ip interface\r\n");
                str += _("====================================================================\r\n");
                str += _("To get more information about a specific command issue 'HELP comman'\r\n");
                str += _("+                 - Repeat last command.\r\n");
                str += _("NOOP              - No operation. Does nothing.\r\n");
                str += _("QUIT              - Close the connection.\r\n");
                str += _("USER 'username'   - Username for login. \r\n");
                str += _("PASS 'password'   - Password for login.  \r\n");
                str += _("CHALLENGE 'token' - Get session id.  \r\n");
                str += _("SEND 'event'      - Send an event.   \r\n");
                str += _("RETR 'count'      - Retrive n events from input queue.   \r\n");
                str += _("RCVLOOP           - Will retrieve events in an endless loop until the connection is closed by the client or QUITLOOP is sent.\r\n");
                str += _("QUITLOOP          - Terminate RCVLOOP.\r\n");
                str += _("CDTA/CHKDATA      - Check if there is data in the input queue.\r\n");
                str += _("CLRA/CLRALL       - Clear input queue.\r\n");
                str += _("STAT              - Get statistical information.\r\n");
                str += _("INFO              - Get status info.\r\n");
                str += _("CHID              - Get channel id.\r\n");
                str += _("SGID/SETGUID      - Set GUID for channel.\r\n");
                str += _("GGID/GETGUID      - Get GUID for channel.\r\n");
                str += _("VERS/VERSION      - Get VSCP daemon version.\r\n");
                str += _("SFLT/SETFILTER    - Set incoming event filter.\r\n");
                str += _("SMSK/SETMASK      - Set incoming event mask.\r\n");
                str += _("HELP [command]    - This command.\r\n");
                str += _("TEST              - Do test sequence. Only used for debugging.\r\n");
                str += _("SHUTDOWN          - Shutdown the daemon.\r\n");
                str += _("RESTART           - Restart the daemon.\r\n");
                str += _("DRIVER            - Driver manipulation.\r\n");
                str += _("FILE              - File handling.\r\n");
                str += _("UDP               - UDP.\r\n");
                str += _("REMOTE            - User handling.\r\n");
                str += _("INTERFACE         - Interface handling. \r\n");
                str += _("DM                - Decision Matrix manipulation.\r\n");
                str += _("VAR               - Variable handling. \r\n");
                str += _("WCYD/WHATCANYOUDO - Check server capabilities. \r\n");
                mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("+") == strcmd ) {
        wxString str = _("'+' repeats the last given command.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("NOOP") == strcmd ) {
        wxString str = _("'NOOP' Does absolutly nothing but giving a success in return.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("QUIT") == strcmd ) {
        wxString str = _("'QUIT' Quit a session with the VSCP daemon and closes the connection.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("USER") == strcmd ) {
        wxString str = _("'USER' Used to login to the system together with PASS. Connection will be closed if bad credentials are given.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("PASS") == strcmd ) {
        wxString str = _("'PASS' Used to login to the system together with USER. Connection will be closed if bad credentials are given.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("QUIT") == strcmd ) {
        wxString str = _("'QUIT' Quit a session with the VSCP daemon and closes the connection.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("SEND") == strcmd ) {
        wxString str = _("'SEND event'.\r\nThe event is given as 'head,class,type,obid,time-stamp,GUID,data1,data2,data3....' \r\n");
        str += _("Normally set 'head' and 'obid' to zero. \r\nIf timestamp is set to zero it will be set by the server. \r\nIf GUID is given as '-' ");
        str += _("the GUID of the interface will be used. \r\nThe GUID should be given on the form MSB-byte:MSB-byte-1:MSB-byte-2. \r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("RETR") == strcmd ) {
        wxString str = _("'RETR count' - Retrieve one (if no argument) or 'count' event(s). ");
        str += _("Events are retrived on the form head,class,type,obid,time-stamp,GUID,data0,data1,data2,...........\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("RCVLOOP") == strcmd ) {
        wxString str = _("'RCVLOOP' - Enter the receive loop and receive events continously or until ");
        str += _("terminated with 'QUITLOOP'. Events are retrived on the form head,class,type,obid,time-stamp,GUID,data0,data1,data2,...........\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("QUITLOOP") == strcmd ) {
        wxString str = _("'QUITLOOP' - End 'RCVLOOP' event receives.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("CDTA") == strcmd ) || ( ( _("CHKDATA") == strcmd ) ) ) {
        wxString str = _("'CDTA' or 'CHKDATA' - Check if there is events in the input queue.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("CLRA") == strcmd ) || ( ( _("CLRALL") == strcmd ) ) ) {
        wxString str = _("'CLRA' or 'CLRALL' - Clear input queue.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("STAT") == strcmd ) {
        wxString str = _("'STAT' - Get statistical information.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("INFO") == strcmd ) {
        wxString str = _("'INFO' - Get status information.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("CHID") == strcmd ) || ( ( _("GETCHID") == strcmd ) ) ) {
        wxString str = _("'CHID' or 'GETCHID' - Get channel id.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("SGID") == strcmd ) || ( ( _("SETGUID") == strcmd ) ) ) {
        wxString str = _("'SGID' or 'SETGUID' - Set GUID for channel.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("GGID") == strcmd ) || ( ( _("GETGUID") == strcmd ) ) ) {
        wxString str = _("'GGID' or 'GETGUID' - Get GUID for channel.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("VERS") == strcmd ) || ( ( _("VERSION") == strcmd ) ) ) {
        wxString str = _("'VERS' or 'VERSION' - Get version of VSCP daemon.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("SFLT") == strcmd ) || ( ( _("SETFILTER") == strcmd ) ) ) {
        wxString str = _("'SFLT' or 'SETFILTER' - Set filter for channel. ");
        str += _("The format is 'filter-priority, filter-class, filter-type, filter-GUID' \r\n");
        str += _("Example:  \r\nSETFILTER 1,0x0000,0x0006,ff:ff:ff:ff:ff:ff:ff:01:00:00:00:00:00:00:00:00\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("SMSK") == strcmd ) || ( ( _("SETMASK") == strcmd ) ) ) {
        wxString str = _("'SMSK' or 'SETMASK' - Set mask for channel. ");
        str += _("The format is 'mask-priority, mask-class, mask-type, mask-GUID' \r\n");
        str += _("Example:  \r\nSETMASK 0x0f,0xffff,0x00ff,ff:ff:ff:ff:ff:ff:ff:01:00:00:00:00:00:00:00:00 \r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("HELP") == strcmd ) {
        wxString str = _("'HELP [command]' This command. Gives help about available commands and the usage.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("TEST") == strcmd ) {
        wxString str = _("'TEST [sequency]' Test command for debugging.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("SHUTDOWN") == strcmd ) {
        wxString str = _("'SHUTDOWN' Shutdown the daemon.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("RESTART") == strcmd ) {
        wxString str = _("'RESTART' Restart the daemon.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("DRIVER") == strcmd ) {
        wxString str = _("'DRIVER' Handle (load/unload/update/start/stop) Level I/Level II drivers.\r\n");
        str += _("'DRIVER install package' .\r\n");
        str += _("'DRIVER uninstall package' .\r\n");
        str += _("'DRIVER upgrade package' .\r\n");
        str += _("'DRIVER start package' .\r\n");
        str += _("'DRIVER stop package' .\r\n");
        str += _("'DRIVER reload package' .\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("FILE") == strcmd ) {
        wxString str = _("'FILE' Handle daemon files.\r\n");
        str += _("'FILE dir'.\r\n");
        str += _("'FILE copy'.\r\n");
        str += _("'FILE move'.\r\n");
        str += _("'FILE delete'.\r\n");
        str += _("'FILE list'.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("UDP") == strcmd ) {
        wxString str = _("'UDP' Handle UDP interface.\r\n");
        str += _("'UDP enable'.\r\n");
        str += _("'UDP disable' .\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("REMOTE") == strcmd ) {
        wxString str = _("'REMOTE' User management.\r\n");
        str += _("'REMOTE list'.\r\n");
        str += _("'REMOTE add 'username','MD5 password','from-host(s)','access-right-list','event-list','filter','mask''. Add a user.\r\n");
        str += _("'REMOTE remove username'.\r\n");
        str += _("'REMOTE privilege 'username','access-right-list''.\r\n");
        str += _("'REMOTE password 'username','MD5 for password' '.\r\n");
        str += _("'REMOTE host-list 'username','host-list''.\r\n");
        str += _("'REMOTE event-list 'username','event-list''.\r\n");
        str += _("'REMOTE filter 'username','filter''.\r\n");
        str += _("'REMOTE mask 'username','mask''.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("INTERFACE") == strcmd ) {
        wxString str = _("'INTERFACE' Handle interfaces on the daemon.\r\n");
        str += _("'INTERFACE list'.\r\n");
        str += _("'INTERFACE close'.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( _("DM") == strcmd ) {
        wxString str = _("'DM' Handle decision matrix on the daemon.\r\n");
        str += _("'DM enable'.\r\n");
        str += _("'DM disable'.\r\n");
        str += _("'DM list'.\r\n");
        str += _("'DM add'.\r\n");
        str += _("'DM delete'.\r\n");
        str += _("'DM reset'.\r\n");
        str += _("'DM clrtrig'.\r\n");
        str += _("'DM clrerr'.\r\n");
        str += _("'DM load'.\r\n");
        str += _("'DM save'.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }
    else if ( ( _("VARIABLE") == strcmd ) || ( _("VAR") == strcmd ) ) {
        wxString str = _("'VARIABLE' Handle variables on the daemon.\r\n");
        str += _("'VARIABLE list <regular-expression>'.\r\n");        
        str += _("'VARIABLE read <variable-name>'.\r\n");
        str += _("'VARIABLE readvalue <variable-name>'.\r\n");
        str += _("'VARIABLE readnote <variable-name>'.\r\n");
        str += _("'VARIABLE write <variable-name> <variable>'.\r\n");
        str += _("'VARIABLE writevalue <variable-name> <value>'.\r\n");
        str += _("'VARIABLE writenote <variable-name>' <note>.\r\n");
        str += _("'VARIABLE reset <variable-name>'.\r\n");
        str += _("'VARIABLE readreset <variable-name>'.\r\n");
        str += _("'VARIABLE remove <variable-name>'.\r\n");
        str += _("'VARIABLE readremove <variable-name>'.\r\n");
        str += _("'VARIABLE length <variable-name>'.\r\n");
        str += _("'VARIABLE save <path> <selection>'.\r\n");
        mg_send( conn, (const char *)str.mbc_str(), str.Length() );
    }

    mg_send( conn, MSG_OK, strlen(MSG_OK) );
    return;
}


///////////////////////////////////////////////////////////////////////////////
// handleClientTest
//

void VSCPClientThread::handleClientTest ( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
    return;
}


///////////////////////////////////////////////////////////////////////////////
// handleClientRestart
//

void VSCPClientThread::handleClientRestart ( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
    return;
}


///////////////////////////////////////////////////////////////////////////////
// handleClientShutdown
//

void VSCPClientThread::handleClientShutdown ( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( !pClientItem->bAuthenticated ) {
        mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
    }

    mg_send( conn,  MSG_GOODBY, strlen ( MSG_GOODBY ) );
    conn->flags |= MG_F_CLOSE_IMMEDIATELY;
    //m_pCtrlObject->m_bQuit = true;
    //m_bRun = false;
}


///////////////////////////////////////////////////////////////////////////////
// handleClientRemote
//

void VSCPClientThread::handleClientRemote( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    return;
}


///////////////////////////////////////////////////////////////////////////////
// handleClientInterface
//
// list     List interfaces.
// unique   Aquire selected interface uniquely. Full format is INTERFACE UNIQUE id
// normal   Normal access to interfaces. Full format is INTERFACE NORMAL id
// close    Close interfaces. Full format is INTERFACE CLOSE id

void VSCPClientThread::handleClientInterface( struct mg_connection *conn, CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    m_pCtrlObject->logMsg ( pClientItem->m_currentCommandUC, DAEMON_LOGMSG_NORMAL );

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "LIST" ) ) )  {
        handleClientInterface_List( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "UNIQUE " ) ) )  {
        pClientItem->m_currentCommandUC = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-7 );            // Remove "UNIQUE "
        handleClientInterface_Unique( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "NORMAL" ) ) )   {
        handleClientInterface_Normal( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "CLOSE" ) ) )    {
        handleClientInterface_Close( conn, pCtrlObject );
    }
}

///////////////////////////////////////////////////////////////////////////////
// handleClientInterface_List
//

void VSCPClientThread::handleClientInterface_List( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    wxString strGUID;
    wxString strBuf;

    // Display Interface List
    m_pCtrlObject->m_wxClientMutex.Lock();
    VSCPCLIENTLIST::iterator iter;
    for (iter = m_pCtrlObject->m_clientList.m_clientItemList.begin();
        iter != m_pCtrlObject->m_clientList.m_clientItemList.end();
        ++iter) {

            CClientItem *pItem = *iter;
            //writeGuidArrayToString( pItem->m_GUID, strGUID );	// Get GUID
            pItem->m_guid.toString( strGUID );
            strBuf = wxString::Format(_("%d,"), pItem->m_clientID );
            strBuf += wxString::Format(_("%d,"), pItem->m_type );
            strBuf += strGUID;
            strBuf += _(",");
            strBuf += pItem->m_strDeviceName;
            strBuf += _("\r\n");

            mg_send( conn,  strBuf.mb_str(),
                                    strlen( strBuf.mb_str() ) );

    }

    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );

    m_pCtrlObject->m_wxClientMutex.Unlock();
}

///////////////////////////////////////////////////////////////////////////////
// handleClientInterface_Unique
//

void VSCPClientThread::handleClientInterface_Unique( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    unsigned char ifGUID[ 16 ];
    memset( ifGUID, 0, 16 );

    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Get GUID
    pClientItem->m_currentCommandUC.Trim( false );
    vscp_getGuidFromStringToArray( ifGUID, pClientItem->m_currentCommandUC );

    // Add the client to the Client List
    // TODO

    mg_send( conn, MSG_INTERFACE_NOT_FOUND, strlen( MSG_INTERFACE_NOT_FOUND ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleClientInterface_Normal
//

void VSCPClientThread::handleClientInterface_Normal( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    // TODO
}

///////////////////////////////////////////////////////////////////////////////
// handleClientInterface_Close
//

void VSCPClientThread::handleClientInterface_Close( struct mg_connection *conn,
                                                        CControlObject *pCtrlObject )
{
    // TODO
}


///////////////////////////////////////////////////////////////////////////////
// handleClientUdp
//

void VSCPClientThread::handleClientUdp( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    // TODO
}


///////////////////////////////////////////////////////////////////////////////
// handleClientFile
//

void VSCPClientThread::handleClientFile( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    // TODO
}


///////////////////////////////////////////////////////////////////////////////
// handleClientVariable
//

void VSCPClientThread::handleClientVariable( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    //m_pCtrlObject->logMsg ( pClientItem->m_currentCommandUC + _("\n"),
    //                            DAEMON_LOGMSG_DEBUG );

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-4 );    // remove "VAR "
    pClientItem->m_currentCommand =
        pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length()-4 );        // remove "VAR "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommand.Trim( false );

    if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "LIST " ) ) )	{
        handleVariable_List( conn, pCtrlObject );
    }
    // Hack to handle "variable list"
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "LIST" ) ) )	{
        pClientItem->m_currentCommandUC += _(" ");
        pClientItem->m_currentCommand += _(" ");
        handleVariable_List( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "WRITE " ) ) )   {
        handleVariable_Write( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "WRITEVALUE " ) ) )   {
        handleVariable_WriteValue( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "WRITENOTE " ) ) )   {
        handleVariable_WriteNote( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "READ " ) ) )    {
        handleVariable_Read( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "READVALUE " ) ) )   {
        handleVariable_ReadValue( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "READNOTE " ) ) )   {
        handleVariable_ReadNote( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "READRESET " ) ) )   {
        handleVariable_ReadReset( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "RESET" ) ) )    {
        handleVariable_Reset( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "REMOVE " ) ) )  {
        handleVariable_Remove( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "READREMOVE " ) ) )  {
        handleVariable_ReadRemove( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "LENGTH " ) ) )  {
        handleVariable_Length( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "LOAD" ) ) ) {
        handleVariable_Load( conn, pCtrlObject );
    }
    else if ( 0 == pClientItem->m_currentCommandUC.Find ( wxT( "SAVE" ) ) ) {
        handleVariable_Save( conn, pCtrlObject );
    }
    else {
        mg_send( conn, MSG_UNKNOWN_COMMAND, strlen( MSG_UNKNOWN_COMMAND ) );
    }
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_List
//
// variable list - List all variables.
// variable list test - List all variables with "test" in there name
//

void VSCPClientThread::handleVariable_List( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    CVSCPVariable variable;
    wxString wxstr;
    wxString strWork;
    wxString strSearch;
    
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-5 );    // remove "LIST "
    pClientItem->m_currentCommandUC.Trim();
    pClientItem->m_currentCommandUC.Trim( false );

    wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(" ") );
    if ( tkz.HasMoreTokens() ) {
        wxString token = tkz.GetNextToken();
        token.Trim();
        token.Trim( false );
        if ( !token.empty() ) {
            strSearch = token;
        }
        else {
            strSearch = _("(.*))");     // List all
        }
    }
    else {
        strSearch = _("(.*)");          // List all
    }

    wxArrayString arrayVars;
    m_pCtrlObject->m_VSCP_Variables.getVarlistFromRegExp( arrayVars, strSearch );
    
    if ( arrayVars.Count() ) {
        
        wxstr = wxString::Format( _("%zu rows.\r\n"), arrayVars.Count() );
        mg_send( conn,  wxstr.mb_str(), wxstr.Length() );
    
        for ( int i=0; i<arrayVars.Count(); i++ ) {
            if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( arrayVars[ i ], variable ) ) {
                wxstr = wxString::Format( _("%d;"), i );
                wxstr += variable.getAsString();
                wxstr += _("\r\n");
                mg_send( conn,  wxstr.mb_str(), wxstr.Length() );
            }
        }
    
    }
    else {
        wxstr = _("0 rows.\r\n");
        mg_send( conn,  wxstr.mb_str(), wxstr.Length() );
    } 

    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_Write
//
// Format: variablename;variabletype;accessrights;persistance;value;note 
// testdata: 
//  test31;string;777;1;dGhpcyBpcyBhIHRlc3Q=;VGhpcyBpcyBhIG5vdGUgZm9yIGEgdGVzdCB2YXJpYWJsZQ==
//  test32;string;777;true;dGhpcyBpcyBhIHRlc3Q=;VGhpcyBpcyBhIG5vdGUgZm9yIGEgdGVzdCB2YXJpYWJsZQ==
//

void VSCPClientThread::handleVariable_Write( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    CVSCPVariable variable;
    wxString wxstr;

    
    bool bPersistence = false;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommand =
        pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length()-6 ); // remove "WRITE "
    pClientItem->m_currentCommand.Trim( false );
    pClientItem->m_currentCommand.Trim( true );
    
    // Also fix UC version
    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-6 );    // remove "WRITE "
    pClientItem->m_currentCommand.Trim();
    pClientItem->m_currentCommand.Trim( false );

    wxStringTokenizer tkz( pClientItem->m_currentCommand, _(";") );

    // Name
    if ( tkz.HasMoreTokens() ) {
        
        wxstr = tkz.GetNextToken();
        wxstr.MakeUpper();
        wxstr.Trim();
        if ( wxstr.IsEmpty() ) {
            // Must have a name
            mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
            return;
        }
        
        if ( !variable.setName( wxstr ) ) {
            // Invalid name
            mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
            return;
        }
        
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    // type - string default. Can also be numerical
    if ( tkz.HasMoreTokens() ) {
        
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        wxstr.Trim( false );
        if ( wxstr.IsEmpty() ) {
            wxstr = _("STRING");   // String is default type
        }
        
        variable.setType( wxstr );
        
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    // Set user to current user
    variable.setUserId( pClientItem->m_pUserItem->getUserID() );
    
    // access-rights 
    if ( tkz.HasMoreTokens() ) {
        
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        if ( wxstr.IsEmpty() ) {
            // Set default
            wxstr = _("744");
        }
        
        unsigned long lval;
        if ( !wxstr.ToULong( &lval ) ) {
            // Not numerical
            mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
            return;
        }
        
        variable.setAccessRights( lval );
        
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    // persistence - false default
    if ( tkz.HasMoreTokens() ) {
        
        wxstr = tkz.GetNextToken();
        wxstr.Trim();
        
        if ( wxstr.IsEmpty() ) {
            // Set default
            wxstr = _("FALSE"); 
        }
        
        unsigned long lval;
        if ( wxstr.ToULong( &lval ) ) {
            
            if ( lval ) {
                variable.setPersistent( true );
            }
            else {
                variable.setPersistent( false );
            }
            
        }
        else {
            
            wxstr.MakeUpper();
            if ( wxNOT_FOUND != wxstr.Find( _("TRUE") ) ) {
                variable.setPersistent( true );
            }
            else {
                variable.setPersistent( false );
            }
        
        }
        
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    // Value
    if ( tkz.HasMoreTokens() ) {
        
        wxstr = tkz.GetNextToken();       
        wxstr.Trim();
        
        variable.setValue( wxstr );
        
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    // Note
    if ( tkz.HasMoreTokens() ) {
        
        wxstr = tkz.GetNextToken();       
        wxstr.Trim();
        
        variable.setNote( wxstr );
        
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    if ( m_pCtrlObject->m_VSCP_Variables.exist( variable.getName() ) ) {
        
        // Update in database
        if ( !m_pCtrlObject->m_VSCP_Variables.update( variable ) ) {
            mg_send( conn, MSG_VARIABLE_NO_SAVE, strlen ( MSG_VARIABLE_NO_SAVE ) );
            return;
        }
        
    }
    else {
       
        // If the variable exist change value
        // if not - add it.
        m_pCtrlObject->m_VSCP_Variables.add( variable ); 
        
    }
    
    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_WriteValue
//

void VSCPClientThread::handleVariable_WriteValue( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    wxString name;
    wxString value;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;
    
    pClientItem->m_currentCommand =
        pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length()-11 ); // remove "WRITEVALUE "
    pClientItem->m_currentCommand.Trim( false );
    pClientItem->m_currentCommand.Trim( true );

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-11 ); // remove "WRITEVALUE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );
    
    wxStringTokenizer tkz( pClientItem->m_currentCommand, _(" ") );
    
    // Variable name
    if ( tkz.HasMoreTokens() ) {
        name = tkz.GetNextToken();
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    // Variable value
    if ( tkz.HasMoreTokens() ) {
        value = tkz.GetNextToken();
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( name, variable ) ) {
        
        // Set value and encode as BASE64 when expected
        variable.setValueFromString( variable.getType(), value, true );
        
        // Update in database
        if ( !m_pCtrlObject->m_VSCP_Variables.update( variable ) ) {
            mg_send( conn, MSG_VARIABLE_NO_SAVE, strlen ( MSG_VARIABLE_NO_SAVE ) );
            return;
        }
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }
    
    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_WriteNote
//

void VSCPClientThread::handleVariable_WriteNote( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;
    
    pClientItem->m_currentCommand =
        pClientItem->m_currentCommand.Right( pClientItem->m_currentCommand.Length()-10 ); // remove "WRITENOTE "
    pClientItem->m_currentCommand.Trim( false );
    pClientItem->m_currentCommand.Trim( true );

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-10 ); // remove "WRITENOTE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );
    
    wxString name;
    wxString note;
    
    wxStringTokenizer tkz( pClientItem->m_currentCommand, _(" ") );
    
    // Variable name
    if ( tkz.HasMoreTokens() ) {
        name = tkz.GetNextToken();
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }
    
    // Variable value
    if ( tkz.HasMoreTokens() ) {
        note = tkz.GetNextToken();
    }
    else {
        mg_send( conn, MSG_PARAMETER_ERROR, strlen( MSG_PARAMETER_ERROR ) );
        return;
    }

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( name, variable ) ) {
        
        // Set value and encode as BASE64 when expected
        variable.setNote( note, true );
        
        // Update in database
        if ( !m_pCtrlObject->m_VSCP_Variables.update( variable ) ) {
            mg_send( conn, MSG_VARIABLE_NO_SAVE, strlen ( MSG_VARIABLE_NO_SAVE ) );
            return;
        }
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }
    
    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_Read
//

void VSCPClientThread::handleVariable_Read( struct mg_connection *conn,
                                                CControlObject *pCtrlObject, 
                                                bool bOKResponse )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-5 ); // remove "READ "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC,variable ) ) {
        
        str = variable.getAsString( false );
        str = _("+OK - ") + str + _("\r\n");
        mg_send( conn,  str.mbc_str(), strlen( str.mbc_str() ) );

        mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }
    
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_ReadVal
//

void VSCPClientThread::handleVariable_ReadValue( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject, 
                                                    bool bOKResponse )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-10 ); // remove "READVALUE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC,variable ) ) {
        
        variable.writeValueToString( str );
        str = _("+OK - ") + str + _("\r\n");
        mg_send( conn,  str.mbc_str(), strlen( str.mbc_str() ) );

        mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }
}


///////////////////////////////////////////////////////////////////////////////
// handleVariable_ReadNote
//

void VSCPClientThread::handleVariable_ReadNote( struct mg_connection *conn,
                                                CControlObject *pCtrlObject, bool bOKResponse )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-9 ); // remove "READNOTE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC,variable ) ) {
        
        str = variable.getNote();
        str = _("+OK - ") + str + _("\r\n");
        mg_send( conn,  str.mbc_str(), strlen( str.mbc_str() ) );

        mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_Reset
//

void VSCPClientThread::handleVariable_Reset( struct mg_connection *conn,
                                                CControlObject *pCtrlObject  )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-6 ); // remove "RESET "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );

    CVSCPVariable variable;
    
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC, variable ) ) {
        
        variable.Reset();
        
        // Update in database
        if ( !m_pCtrlObject->m_VSCP_Variables.update( variable ) ) {
            mg_send( conn, MSG_VARIABLE_NO_SAVE, strlen ( MSG_VARIABLE_NO_SAVE ) );
            return;
        }
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_ReadReset
//

void VSCPClientThread::handleVariable_ReadReset( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-10 ); // remove "READRESET "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );
    
    if ( pClientItem->m_currentCommandUC.StartsWith( _("VSCP.") ) ) {
        mg_send( conn, MSG_VARIABLE_NOT_STOCK, strlen ( MSG_VARIABLE_NOT_STOCK ) );
        return;
    }

    CVSCPVariable variable;
    
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC, variable ) ) {
        
        variable.writeValueToString( str );
        str = _("+OK - ") + str + _("\r\n");
        mg_send( conn,  str.mbc_str(), strlen( str.mbc_str() ) );
    
        variable.Reset();
        
        // Update in database
        if ( !m_pCtrlObject->m_VSCP_Variables.update( variable ) ) {
            mg_send( conn, MSG_VARIABLE_NO_SAVE, strlen ( MSG_VARIABLE_NO_SAVE ) );
            return;
        }
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}


///////////////////////////////////////////////////////////////////////////////
// handleVariable_Remove
//

void VSCPClientThread::handleVariable_Remove( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-7 ); // remove "REMOVE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );
    
    if ( pClientItem->m_currentCommandUC.StartsWith( _("VSCP.") ) ) {
        mg_send( conn, MSG_VARIABLE_NOT_STOCK, strlen ( MSG_VARIABLE_NOT_STOCK ) );
        return;
    }
    
    if ( !m_pCtrlObject->m_VSCP_Variables.remove( pClientItem->m_currentCommandUC ) ) {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );        
    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}


///////////////////////////////////////////////////////////////////////////////
// handleVariable_ReadRemove
//

void VSCPClientThread::handleVariable_ReadRemove( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-11 ); // remove "READREMOVE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );
    
    if ( pClientItem->m_currentCommandUC.StartsWith( _("VSCP.") ) ) {
        mg_send( conn, MSG_VARIABLE_NOT_STOCK, strlen ( MSG_VARIABLE_NOT_STOCK ) );
        return;
    }

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC, variable ) ) {
        
        variable.writeValueToString( str );
        str = _("+OK - ") + str + _("\r\n");
        mg_send( conn,  str.mbc_str(), strlen( str.mbc_str() ) );
    
        m_pCtrlObject->m_VSCP_Variables.remove( variable );
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleVariable_Length
//

void VSCPClientThread::handleVariable_Length( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    wxString str;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-7 ); // remove "LENGTH "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );

    CVSCPVariable variable;
    if ( 0 != m_pCtrlObject->m_VSCP_Variables.find( pClientItem->m_currentCommandUC, variable ) ) {
        
        str = wxString::Format( _("%zu"), variable.getLength() );
        str = _("+OK - ") + str + _("\r\n");
        mg_send( conn,  str.mbc_str(), strlen( str.mbc_str() ) );
    
        m_pCtrlObject->m_VSCP_Variables.remove( variable );
        
    }
    else {
        mg_send( conn, MSG_VARIABLE_NOT_DEFINED, strlen ( MSG_VARIABLE_NOT_DEFINED ) );
    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}



///////////////////////////////////////////////////////////////////////////////
// handleVariable_Load
//

void VSCPClientThread::handleVariable_Load( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    wxString path;  // Empty to load from default path
    m_pCtrlObject->m_VSCP_Variables.loadFromXML( path );  // TODO add path + type

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}


///////////////////////////////////////////////////////////////////////////////
// handleVariable_Save
//

void VSCPClientThread::handleVariable_Save( struct mg_connection *conn,
                                                CControlObject *pCtrlObject )
{
    wxString path;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    pClientItem->m_currentCommandUC = 
            pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-5 ); // remove "SAVE "
    pClientItem->m_currentCommandUC.Trim( false );
    pClientItem->m_currentCommandUC.Trim( true );

    // Construct path to save to (always relative to root)
    // may not contain ".."
    path = m_pCtrlObject->m_rootFolder;
    path += pClientItem->m_currentCommandUC;
    
    if ( wxNOT_FOUND != path.Find( _("..") ) ) {
        mg_send( conn, MSG_INVALID_PATH, strlen( MSG_INVALID_PATH ) );
        return;
    }

    m_pCtrlObject->m_VSCP_Variables.save( path );

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}




// ****************************************************************************



///////////////////////////////////////////////////////////////////////////////
// handleClientDm
//

void VSCPClientThread::handleClientDm( struct mg_connection *conn,
                                        CControlObject *pCtrlObject  )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    m_pCtrlObject->logMsg ( pClientItem->m_currentCommandUC, DAEMON_LOGMSG_NORMAL );

    pClientItem->m_currentCommandUC = pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length()-3 );
    pClientItem->m_currentCommandUC.Trim( false );

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ENABLE " ) ) )   {
        handleDM_Enable( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "DISABLE " ) ) ) {
        handleDM_Enable( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "LIST" ) ) ) {
        handleDM_List( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ADD " ) ) ) {
        handleDM_Add( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "DELETE " ) ) )  {
        handleDM_Delete( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "RESET" ) ) )    {
        handleDM_Reset( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "TRIG " ) ) )    {
        handleDM_Trigger( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "CLRTRIG " ) ) ) {
        handleDM_ClearTriggerCount( conn, pCtrlObject );
    }
    else if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "CLRERR " ) ) )  {
        handleDM_ClearErrorCount( conn, pCtrlObject );
    }
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_Enable
//

void VSCPClientThread::handleDM_Enable( struct mg_connection *conn,
                                            CControlObject *pCtrlObject  )
{
    unsigned short pos;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ALL" ) ) )   {

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        DMLIST::iterator iter;
        for (iter = m_pCtrlObject->m_dm.m_DMList.begin(); iter != m_pCtrlObject->m_dm.m_DMList.end(); ++iter)
        {
            dmElement *pDMItem = *iter;
            pDMItem->enableRow();
        }

        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }
    else {

        // Get the position
        pos = 0;

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        if ( pos > ( m_pCtrlObject->m_dm.m_DMList.GetCount() - 1 ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            m_pCtrlObject->m_dm.m_mutexDM.Unlock();
            return;
        }

        DMLIST::compatibility_iterator node = m_pCtrlObject->m_dm.m_DMList.Item( pos );
        ( node->GetData() )->enableRow();
        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleDM_Disable
//

void VSCPClientThread::handleDM_Disable( struct mg_connection *conn,
                                            CControlObject *pCtrlObject  )
{
    unsigned short pos;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ALL" ) ) )   {

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        DMLIST::iterator iter;
        for (iter = m_pCtrlObject->m_dm.m_DMList.begin();
            iter != m_pCtrlObject->m_dm.m_DMList.end();
            ++iter )
        {
            dmElement *pDMItem = *iter;
            pDMItem->disableRow();
        }
        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }
    else {

        // Get the position
        wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(",") );
        while ( tkz.HasMoreTokens() ) {

            pos = vscp_readStringValue( tkz.GetNextToken() );

            m_pCtrlObject->m_dm.m_mutexDM.Lock();

            if ( pos > ( m_pCtrlObject->m_dm.m_DMList.GetCount() - 1 ) ) {
                mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
                m_pCtrlObject->m_dm.m_mutexDM.Unlock();
                return;
            }

            DMLIST::compatibility_iterator node = m_pCtrlObject->m_dm.m_DMList.Item( pos );
            ( node->GetData() )->disableRow();

            m_pCtrlObject->m_dm.m_mutexDM.Unlock();

        }

    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_List
//

void VSCPClientThread::handleDM_List( struct mg_connection *conn,
                                        CControlObject *pCtrlObject  )
{
    // Valid commands at this point
    // dm list
    // dm list all
    // dm list *
    // dm list 1
    // dm list 1,2,3,4,98

    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Remove "LIST"
    pClientItem->m_currentCommandUC =
        pClientItem->m_currentCommandUC.Right( pClientItem->m_currentCommandUC.Length() - 4 );
    pClientItem->m_currentCommandUC.Trim( false );

    // if "list" add "all"
    if ( 0 == pClientItem->m_currentCommandUC.Length() ) {
        pClientItem->m_currentCommandUC = _("ALL");
    }

    if ( ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ALL" ) ) ) ||
            ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "*" ) ) ) )	{

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        DMLIST::iterator iter;
        for (iter = m_pCtrlObject->m_dm.m_DMList.begin();
                iter != m_pCtrlObject->m_dm.m_DMList.end();
                ++iter) {

            dmElement *pDMItem = *iter;
            wxString strRow = pDMItem->getAsString();

            mg_send( conn,  strRow.mb_str(),
                                        strlen ( strRow.mb_str() ) );

        }

        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }
    else {

        // We have a list with specific rows  "list 1,8,9"
        //      first parse the argument to get the rows
        //WX_DEFINE_ARRAY_INT( int, ArrayOfSortedInts );
        wxArrayInt rowArray;
        wxStringTokenizer tok( pClientItem->m_currentCommandUC, _(",") );
        while ( tok.HasMoreTokens() ) {

            int n = vscp_readStringValue( tok.GetNextToken() );
            rowArray.Add( n );

        }

    }


    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );

}

///////////////////////////////////////////////////////////////////////////////
// handleDM_Add
//

void VSCPClientThread::handleDM_Add( struct mg_connection *conn,
                                        CControlObject *pCtrlObject  )
{
    CClientItem *pClientItem = (CClientItem *)conn->user_data;
    dmElement *pDMItem = new dmElement;

    wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(",") );

    // Priority
    pDMItem->m_vscpfilter.mask_priority = vscp_readStringValue( tkz.GetNextToken() );
    pDMItem->m_vscpfilter.filter_priority = vscp_readStringValue( tkz.GetNextToken() );

    // Class
    pDMItem->m_vscpfilter.mask_class = vscp_readStringValue( tkz.GetNextToken() );
    pDMItem->m_vscpfilter.filter_class = vscp_readStringValue( tkz.GetNextToken() );

    // Type
    pDMItem->m_vscpfilter.mask_type = vscp_readStringValue( tkz.GetNextToken() );
    pDMItem->m_vscpfilter.filter_type = vscp_readStringValue( tkz.GetNextToken() );

    // GUID
    wxString strGUID;
    strGUID =tkz.GetNextToken();
    vscp_getGuidFromStringToArray( pDMItem->m_vscpfilter.mask_GUID, strGUID );
    strGUID = tkz.GetNextToken();
    vscp_getGuidFromStringToArray( pDMItem->m_vscpfilter.filter_GUID, strGUID );

    // action code
    pDMItem->m_actionCode = vscp_readStringValue( tkz.GetNextToken() );

    // action parameters
    pDMItem->m_actionparam = tkz.GetNextToken();

    // add the DM row to the matrix
    m_pCtrlObject->m_dm.m_mutexDM.Lock();
    m_pCtrlObject->m_dm.addMemoryElement ( pDMItem );
    m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_Delete
//

void VSCPClientThread::handleDM_Delete( struct mg_connection *conn,
                                            CControlObject *pCtrlObject  )
{
    unsigned short pos;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ALL" ) ) )   {

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        DMLIST::iterator iter;
        for (iter = m_pCtrlObject->m_dm.m_DMList.begin();
            iter != m_pCtrlObject->m_dm.m_DMList.end();
            ++iter) {
                dmElement *pDMItem = *iter;
                delete pDMItem;
        }
        m_pCtrlObject->m_dm.m_DMList.Clear();
        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }
    else {

        // Get the position
        wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(",") );
        while ( tkz.HasMoreTokens() ) {

            pos = vscp_readStringValue( tkz.GetNextToken() );

            m_pCtrlObject->m_dm.m_mutexDM.Lock();

            if ( pos > ( m_pCtrlObject->m_dm.m_DMList.GetCount() - 1 ) ) {
                mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
                m_pCtrlObject->m_dm.m_mutexDM.Unlock();
                return;
            }

            DMLIST::compatibility_iterator node = m_pCtrlObject->m_dm.m_DMList.Item( pos );
            dmElement *pDMItem = node->GetData();
            m_pCtrlObject->m_dm.m_DMList.DeleteNode( node );    // Delete the node
            delete pDMItem;                                     // Delete the object

            m_pCtrlObject->m_dm.m_mutexDM.Unlock();

        }

    }

    mg_send( conn,  MSG_OK, strlen ( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_Reset
//

void VSCPClientThread::handleDM_Reset( struct mg_connection *conn, CControlObject *pCtrlObject  )
{
    m_pCtrlObject->stopDaemonWorkerThread();
    m_pCtrlObject->startDaemonWorkerThread();

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_Trigger
//

void VSCPClientThread::handleDM_Trigger( struct mg_connection *conn,
                                            CControlObject *pCtrlObject )
{
    unsigned short pos;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    // Get the position
    wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(",") );
    while ( tkz.HasMoreTokens() ) {

        pos = vscp_readStringValue( tkz.GetNextToken() );

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        if ( pos > ( m_pCtrlObject->m_dm.m_DMList.GetCount() - 1 ) ) {
            mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
            m_pCtrlObject->m_dm.m_mutexDM.Unlock();
            return;
        }

        DMLIST::compatibility_iterator node = m_pCtrlObject->m_dm.m_DMList.Item( pos );
        dmElement *pDMItem = node->GetData();
        pDMItem->doAction( NULL );

        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_ClearTriggerCount
//


void VSCPClientThread::handleDM_ClearTriggerCount( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    unsigned short pos;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ALL" ) ) )	{

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        DMLIST::iterator iter;
        for (iter = m_pCtrlObject->m_dm.m_DMList.begin();
            iter != m_pCtrlObject->m_dm.m_DMList.end();
            ++iter) {
                dmElement *pDMItem = *iter;
                pDMItem->m_triggCounter = 0;
        }

        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }
    else {

        // Get the position
        wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(",") );
        while ( tkz.HasMoreTokens() ) {

            pos = vscp_readStringValue( tkz.GetNextToken() );

            m_pCtrlObject->m_dm.m_mutexDM.Lock();

            if ( pos > ( m_pCtrlObject->m_dm.m_DMList.GetCount() - 1 ) ) {
                mg_send( conn,  MSG_PARAMETER_ERROR,
                    strlen ( MSG_PARAMETER_ERROR ) );
                m_pCtrlObject->m_dm.m_mutexDM.Unlock();
                return;
            }

            DMLIST::compatibility_iterator node = m_pCtrlObject->m_dm.m_DMList.Item( pos );
            dmElement *pDMItem = node->GetData();
            pDMItem->m_triggCounter = 0;

            m_pCtrlObject->m_dm.m_mutexDM.Unlock();

        }

    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleDM_ClearErrorCount
//


void VSCPClientThread::handleDM_ClearErrorCount( struct mg_connection *conn,
                                                    CControlObject *pCtrlObject )
{
    unsigned short pos;
    CClientItem *pClientItem = (CClientItem *)conn->user_data;

    if ( wxNOT_FOUND != pClientItem->m_currentCommandUC.Find ( _( "ALL" ) ) )	{

        m_pCtrlObject->m_dm.m_mutexDM.Lock();

        DMLIST::iterator iter;
        for (iter = m_pCtrlObject->m_dm.m_DMList.begin();
            iter != m_pCtrlObject->m_dm.m_DMList.end();
            ++iter) {
                dmElement *pDMItem = *iter;
                pDMItem->m_errorCounter = 0;
        }

        m_pCtrlObject->m_dm.m_mutexDM.Unlock();

    }
    else {

        // Get the position
        wxStringTokenizer tkz( pClientItem->m_currentCommandUC, _(",") );
        while ( tkz.HasMoreTokens() ) {

            pos = vscp_readStringValue( tkz.GetNextToken() );

            m_pCtrlObject->m_dm.m_mutexDM.Lock();

            if ( pos > ( m_pCtrlObject->m_dm.m_DMList.GetCount() - 1 ) ) {
                mg_send( conn,  MSG_PARAMETER_ERROR, strlen ( MSG_PARAMETER_ERROR ) );
                m_pCtrlObject->m_dm.m_mutexDM.Unlock();
                return;
            }

            DMLIST::compatibility_iterator node = m_pCtrlObject->m_dm.m_DMList.Item( pos );
            dmElement *pDMItem = node->GetData();
            pDMItem->m_errorCounter = 0;

            m_pCtrlObject->m_dm.m_mutexDM.Unlock();

        }

    }

    mg_send( conn, MSG_OK, strlen( MSG_OK ) );
}

///////////////////////////////////////////////////////////////////////////////
// handleClientList
//

void VSCPClientThread::handleClientList( struct mg_connection *conn,
                                            CControlObject *pCtrlObject  )
{
    // TODO
    mg_send( conn, MSG_OK, strlen ( MSG_OK ) );
}


///////////////////////////////////////////////////////////////////////////////
// handleClientDriver
//

void VSCPClientThread::handleClientDriver( struct mg_connection *conn,
                                                CControlObject *pCtrlObject  )
{
    // TODO
}

