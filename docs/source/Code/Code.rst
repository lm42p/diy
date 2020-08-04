====
Code
====

  .. |CodeArduino| raw:: html

    <a href="https://github.com/lm42p/diy/blob/master/docs/source/Code/"
    target="_blank">Code Arduio</a>


Here is the Arduino code. You can download it from here :
|CodeArduino|. 
I'm not a professional programmer. The code is a bit
messy sorry about that. At the begining I tried to use AccelStepper
library for Arduino but couldn't get it work well. (couldn't reach an
appropriate speeed I don't know why) so I remove AccelStepper library
and tried to do something and it worked not badly. Sorry the comments
in the code are in french, I'll translate them soon!

Maybe this video can help to understand the code

.. raw:: html

    <iframe width="350" height="245"
    src="https://www.youtube.com/embed/VgY4AlmjkgQ"
    frameborder="0" 
    allowfullscreen></iframe>

.. literalinclude:: /Code/CodeGeckoV4.ino
   :language: c

Updates
-------

V3 -> V4 : In the V3 when the RJ45 (Remote Control) is pull out
during full speed, the stroke goes to its max. V4 avoid this and a
reset is necessarily. Also V4 doesn't permit a too much high gradient
of stroke change.  
	      

