#!/bin/sh 
#####################################################################
###  File:              0018.xprint
###
###  Default Location:  /usr/dt/config/Xsession.d/
###
###  Purpose:           Setup Xprint env vars
###                     
###  Description:       This script is invoked by means of the Xsession file
###                     at user login. 
###
###  Invoked by:        /usr/dt/bin/Xsession
###
###  (c) Copyright 2003-2004 Roland Mainz <roland.mainz@nrubsig.org>
###
###  please send bugfixes or comments to http://xprint.mozdev.org/
###
#####################################################################


#
# Obtain list of Xprint servers
#

if [ -x "/etc/init.d/xprint" ] ; then
  XPSERVERLIST="`/etc/init.d/xprint get_xpserverlist`"
  export XPSERVERLIST
fi

##########################         eof       #####################
