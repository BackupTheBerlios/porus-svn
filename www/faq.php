<?php $title="about"; include 'header.php'; ?>

<h1>PORUS Frequently Asked Questions</h1>

<div class="faq">

<h1>Introduction</h1>

<h2>What is PORUS?</h2>

<p>PORUS is a portable USB stack for embedded systems.  It implements as much of the code needed for a USB peripheral as possible, including enumeration and standard control request handling.</p>

<h2>What makes PORUS portable?</h2>

<p>PORUS has a core, generated code, and a hardware abstraction layer (HAL).  The HAL contains functions specific to the port.  Most of these functions are very simple, and manipulate the USB hardware directly.</p>

<p>The HAL makes PORUS portable.  The rest of PORUS contains as many of the fiddly bits as possible, and is written in platform-independent ANSI C.</p>

<h2>How much memory does PORUS require?</h2>

<p>It's too early to tell.  Development on the core isn't finished, and PORUS has been ported to only one system.  On different systems, PORUS will take different amounts of ROM and RAM.  We do want to make PORUS as lean as possible, and we want it to run on small microcontrollers.</p>

<h2>Does PORUS pass the relevant USB-IF tests?</h2>

<p>Our goal is for devices (correctly) using PORUS to pass <a href="http://www.usb.org/developers/tools/">USBCV</a>'s tests.</p>

<h2>What does PORUS cost?</h2>

<p>PORUS itself doesn't cost any money, and it's open source.  This means that, while we aren't charging for it, if you change it or modify it, you must make your changed version available to the public.</p>

<h2>Can I use PORUS in my commercial product?</h2>

<p>Yes!  However, if you change PORUS itself, or if you change one of its ports, you have to return the changes back to the main PORUS tree.</p>

<h2>My device uses super special top secret proprietary USB hardware, and I can't release code that talks to it.  Can I still use PORUS?</h2>

<p>Yes, if you make your own port.  Ports are not part of PORUS, and need not carry the same licence.  However, if you change the PORUS core, you still must release the changes.</p>

<h2>Why are you giving PORUS away?</h2>

<p>Because I'm not in the software business.</p>

<h2>What does the name PORUS mean?</h2>

<p>It's a cheap acronym.  It stands for PORtable USB Stack.  It also can mean "pure" in Latin.  (The classical Latin word is <i>purus</i>, but in ancient Latin, an internal 'o' was sometimes written as 'u'.)</p>

</div>

<?php include 'footer.php'; ?>
