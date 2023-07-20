Welcome to JANA!
=================
JANA is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics) event reconstruction. It is very efficient at multi-threading with a design that makes it easy for less experienced programmers to contribute pieces to the larger reconstruction project. The same JANA program can be used to easily do partial or full reconstruction, fully maximizing the available cores for the current job.

Its design strives to be extremely easy to setup when first getting started, yet have a depth of customization options that allow for more complicated reconstruction as your project grows. The intent is to make it easy to run on a laptop for local code development, but to also be highly efficent when deploying to large computing sites like `NERSC <http://www.nersc.gov/>`_.

JANA has undergone a large rewrite with the newer version dubbed JANA2. The code is now available for use and you are free to browse around. The project is hosted on `GitHub <https://github.com/JeffersonLab/JANA2>`_

.. code-block:: console 

   auto tracks = jevent->Get<DTrack>();
   
   for(auto t : tracks){
     // ... do something with a track
   }

Contents
--------

.. toctree::

   tutorial
   how-to guides
   principles
   reference
   how-to instructions


