# X11_corner_barrier

This application prevents cursor movement to adjacent monitors near the corners by employing XPointerBarriers.

The barriers act as one-way barriers and only prevent movement if the cursor is within 100 px of a corner and is currently located on the monitor associated with that corner. 

The code is adapted from the example in https://who-t.blogspot.com/2012/12/whats-new-in-xi-23-pointer-barrier.html
