mx5000tools-revo
================

Linux tools for the MX5000 series keyboard and Revolution Mouse

Pulled from: http://maemo.cloud-7.de/mx-revolution/

Just putting it here for history and ease of access.

=============================================

=== revocon 0.5.1jr (from  http://goron.de/~froese/revoco/revoco-0.5.tar.gz ) ===
tested and working with both MX-revolution (M/N: M-RBQ124) and MX5500-mouse (M/N: M-RCL124)
  got small bugfixes by me to make battery and mode command work


=== mx5000tools 0.1.2 (from http://download.gna.org/mx5000tools/) ===
  ([partially] tested with MX5500-kbd)
  
  ./configure needs "zypper install libnetpbm-devel" on my OpenSuse11

working:
  mx5000-tool --beep
  mx5000-tool --time *)
*): partially working, has bugs. See below

todo:
  fix bug "always 2007" in "mx5000-tool --time
  fix weird --help of mx5000-tool

