FCamera
==========
A sample application for the FCam API.

FCamera is a completely open-source N900 camera implementation written
using the FCam camera control API. We think you should be able to
program your camera to behave any way you want it to. Don't like our
exposure metering algorithm? Change it. Want a mode for doing
time-lapse photography? Write it. Do you desperately want a sepia
toned viewfinder? Go nuts. With FCamera, you can. To get started,
check out our webpage at

http://fcam.garage.maemo.org.

Quick building notes
======================
FCamera depends on fcam-dev, the FCam API development headers and library.

If you're building FCamera inside scratchbox from Maemo extras-* sources, 
install fcam-dev from Maemo extras-testing (or extras, if it's gotten there), 
and ignore the notice about missing fcamera.local.pro.

If you're building FCamera using the Nokia QT SDK, or if you've just
checked out fcam from the Maemo garage SVN repository, copy
fcamera.local.template.pro to fcamera.local.pro. Edit
fcamera.local.pro to point to the location of fcam-dev in your
system.


