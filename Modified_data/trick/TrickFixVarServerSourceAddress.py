##############################################################################
# PURPOSE:
#    (This is a python file for use in a Trick input file.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (June 2025) (--) (Initial implementation.)))
##############################################################################
import socket
import subprocess

def fix_var_server_source_address():
   # The Trick variable server uses the local host name without verifying the
   # IP address it resolves to is actually used by the local host computer.
   # Verify the IP address and fallback to 127.0.0.1 if we find a discrepancy.
   # Otherwise the simulation control panel will not successfully connect.
   try:
      if ( trick.var_server_get_hostname() == socket.gethostname() ):
         host_ip_addr = socket.gethostbyname( socket.gethostname() )
         try:
            ifconfig_out = subprocess.check_output( ['ifconfig'] ).decode()
            if ( ifconfig_out.find( host_ip_addr ) < 0 ):
               print( 'WARNING: Invalid IP address ' + host_ip_addr
                      + ' resolved for host \'' + trick.var_server_get_hostname()
                      + '\', setting the variable server source address to 127.0.0.1!' )
               trick.var_server_set_source_address( '127.0.0.1' )
         except:
            return  # Use host source address as is.
   except ( socket.error, socket.gaierror, socket.herror, socket.timeout ):
      print( 'WARNING: Problem resolving \'' + trick.var_server_get_hostname()
             + '\' host name to an address, setting the variable server source address to 127.0.0.1!' )
      trick.var_server_set_source_address( '127.0.0.1' )
   return
