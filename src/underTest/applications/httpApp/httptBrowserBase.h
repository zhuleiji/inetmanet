
// ***************************************************************************
//
// HttpTools Project
//// This file is a part of the HttpTools project. The project was created at
// Reykjavik University, the Laboratory for Dependable Secure Systems (LDSS).
// Its purpose is to create a set of OMNeT++ components to simulate browsing
// behaviour in a high-fidelity manner along with a highly configurable
// Web server component.
//
// Maintainer: Kristjan V. Jonsson (LDSS) kristjanvj@gmail.com
// Project home page: code.google.com/p/omnet-httptools
//
// ***************************************************************************
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// ***************************************************************************


#ifndef __httptBrowserBase_H_
#define __httptBrowserBase_H_

#include <omnetpp.h>
#include "httptNodeBase.h"

#define MSGKIND_START_SESSION 	 0
#define MSGKIND_NEXT_MESSAGE  	 1
#define MSGKIND_SCRIPT_EVENT  	 2
#define MSGKIND_ACTIVITY_START   3

#define MAX_URL_LENGTH 2048 // The maximum allowed URL string length.

using namespace std;

/**
 * @brief Browse event item. Used in scripted mode.
 */
struct BROWSE_EVENT_ENTRY
{
	simtime_t time;					//> Event triggering time
	string wwwhost;					//> Host to contact
	string resourceName;			//> The resource to request
	httptNodeBase *serverModule;	//> Reference to the omnet server object. Resolved at parse time.
};

/**
 * @brief Browse events queue. Used in scripted mode.
 */
typedef deque<BROWSE_EVENT_ENTRY> BROWSE_EVENT_QUEUE_TYPE;

/**
 * @short Browser module for OMNeT++ simulations - base class
 *
 * A simulated browser module for OMNeT++ simulations. A part of HttpTools.
 *
 * The component is designed to plug into the existing INET StandardHost module as a
 * tcpApp. See the INET documentation and examples for details. It can also be used
 * with the simplified DirectHost, which only supports direct message passing.
 *
 * The browser can operate in two modes:
 * - Random request mode: The browser uses the parameters supplied and statistical distributions
 *   to make requests.
 * - Scripted mode: The browser operates using a script file -- requests are issued to specific
 *   URLs and at specific times.
 *
 * The browser can operate in two communications modes:
 * - Direct mode, in which messages are passed directly using sendDirect. This removes topology
 *   variables from the simulation and simplifies setup considerably. This mode should be used
 *   whenever the topology of the network and the resulting effects are not of interest. This is
 *   implemented in the derived httptBrowserDirect class.
 * - Socket mode, in which the INET TCPSocket is used to handle messages sent and received.
 *   This mode uses the full INET TCP/IP simulation. Requires the network topology to be set
 *   up -- routers, links, etc. This is implemented in the derived httptBrowser class.
 *
 * @see httptBrowser
 * @see httptBrowserDirect
 *
 * @author Kristjan V. Jonsson (kristjanvj@gmail.com)
 * @version 1.0
 */
class INET_API httptBrowserBase : public httptNodeBase
{
	protected:
		cMessage *eventTimer; 			//> The timer object used to trigger browsing events
		httptController *controller; 	//> Reference to the central controller object

		bool scriptedMode; 						//> Set to true if a script file is defined, False otherwise
		BROWSE_EVENT_QUEUE_TYPE browseEvents; 	//> Queue of browse events used in scripted mode

		/** @name The current session parameters */
		//@{
		int reqInCurSession; 			//> The number of requests made sofar in the current session
		int reqNoInCurSession; 			//> The total number of requests to be made in the current session
		double activityPeriodLength; 	//> The length of the currently active activity period
		simtime_t acitivityPeriodEnd; 	//> The end in simulation time of the current activity period
		//@}

		/** @name The random objects */
		//@{
		rdObject *rdProcessingDelay;
		rdObject *rdActivityLength;
		rdObject *rdInterRequestInterval;
		rdObject *rdInterSessionInterval;
		rdObject *rdRequestSize;
		rdObject *rdReqInSession;
		//@}

		/** @name statistics variables */
		//@{
		long htmlRequested;
		long htmlReceived;
		long htmlErrorsReceived;
		long imgResourcesRequested;
		long imgResourcesReceived;
		long textResourcesRequested;
		long textResourcesReceived;
		long messagesInCurrentSession;
		long sessionCount;
		long connectionsCount;
		//@}

	public:
		httptBrowserBase();
		virtual ~httptBrowserBase();

	/** @name cSimpleModule redefinitions */
	//@{
	protected:
		/** Initialization of the component and startup of browse event scheduling */
		virtual void initialize(int stage);

		/** Report final statistics */
		virtual void finish();

		/** Handle incoming messages */
		virtual void handleMessage(cMessage *msg)=0;

		/** @brief Returns the number of initialization stages. Two required. */
		int numInitStages() const {return 2;}

	//@}

	protected:
		/** Handle a HTTP data message */
		void handleDataMessage( cMessage *msg );

		/** Handle a self message -- events and such */
		void handleSelfMessages( cMessage *msg );

		/** @name Handlers for self messages */
		//@{
		void handleSelfActivityStart();							//> Handle start of activity period trigger
		void handleSelfStartSession();							//> Handle start of session trigger
		void handleSelfNextMessage();							//> Handle browse event trigger
		void handleSelfScriptedEvent();							//> Handle a scheduled scripted event
		void handleSelfDelayedRequestMessage(cMessage *msg);	//> Handle a delayed message event
		//@}

		/** Schedule the next browse event. Handles the activity, session and inter-request times */
		void scheduleNextBrowseEvent();

		/** @name pure virtual methods to communicate with the server. Must be implemented in derived classes */
		//@{
		/** Send a request defined by a browse event (scripted entry) to a server */
		virtual void sendRequestToServer( BROWSE_EVENT_ENTRY be )=0;
		/** Send a request to server. Uses the recipient stamped in the request. */
		virtual void sendRequestToServer( httptRequestMessage *request )=0;
		/** Send a request to a randomly selected server. The derived class utilizes the controller object to retrieve the object */
		virtual void sendRequestToRandomServer()=0;
		/** Send a request to a named server */
		virtual void sendRequestsToServer( string www, MESSAGE_QUEUE_TYPE queue )=0;
		//@}

		/** @name Methods for generating HTML page requests and resource requests */
		//@{
		/** Generate a HTTP request to a specific server and for a specific page */
		cMessage* generatePageRequest(string www, string page, bool bad=false, int size=0);
		/** Generate a random HTTP request -- used in case we dont care which page is requested */
		cMessage* generateRandomPageRequest(string www, bool bad=false, int size=0);
		/** Generate a resource request, e.g. for an image or css document */
		cMessage* generateResourceRequest(string www, string resource="", int serial=0, bool bad=false, int size=0);
		//@}

		//* Read scripted events from file. Triggered if the script file parameter is specified in the initialization file. */
		void readScriptedEvents( const char* filename );
};

#endif


