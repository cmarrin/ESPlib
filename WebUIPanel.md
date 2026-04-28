# ESPlib Web UI Panel

ESPlib supports the concepts of Web UI Panels. These are JSON files which are have specifications for a set of widgets to control aspects of whatever functions the ESP is doing. For instance for a controller of LED string lights, it might have an on/off switch and a color selector. You specify these widgets in the JSON file with any layout requirements and what to do when a widget interaction takes place. This is typically sending a HTTP request to the server which executes some Lua control to activate the hardware the widget is controlling.

## Overview

A Web UI Panel consists of a JSON file specifying the widget layout and behavior and a some Lua code to handle the HTTP requests resulting from the interactions with the widgets. A browser makes a request to an endpoint of the form:

	http://<hostname>/uipanel/<panel name>
	
This request looks up a file with the name **\<panel name>.json** and serves an HTML file called uipanel.html which has a js global variable named *panelName* filled in with <panel name>. This HTML file then does an HTTP request for <panel name>.json. When delivered it parses the file and constructs the HTML required to display the requested widgets.

When a widget interaction takes place and XMLHTTPRequest is generated to the endpoint:

	http://<hostname>panel/<panel name>?<params>
	
Params sent are *name* (the **\<panel name>**), *widget* (the widget name), and *value* (the widget value). When received a Lua program with the name **\<panel name>.lua** is executed passing the *widget* and *value* parameters as string arguments. This program then performs whatever operation is appropriate for those arguments.

To insert the **\<panel name>** into *panel.html* a simple token system is used. The panel.html file has a "**{panel-name}**" string which is replaced with the **\<panel name>** using:

	pitem.replace("{panel-name}",  <panel name>);
	
**widget-panel.html** is compiled into the runtime, similar to **landing.html** and others. **\<panel name>.json** and **\<panel name>.lua** are in the **/sys/panels** folder.
