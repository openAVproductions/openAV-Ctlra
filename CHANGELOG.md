Release 0.2 (work in progress)
==============================

Added `ctlra_screen_redraw_request()` function to API, allowing the application
to drive when redraws occur. By setting the screen redraw FPS to zero in the
creation struct, Ctlra will never invoke the screen-redraw callback until
explicitly requested by the application.

Release 0.1 (Jan '21)
=====================

Development milestone release. API is relatively stable due to years of
prototyping on without official releases. API/ABI breaks can still occur.
