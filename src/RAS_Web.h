/*
  OK, ya ready for some fun? HTML + CSS styling + javascript all in and undebuggable environment

  one trick I've learned to how to debug HTML and CSS code.

  get all your HTML code (from html to /html) and past it into this test site
  muck with the HTML and CSS code until it's what you want
  https://www.w3schools.com/html/tryit.asp?filename=tryhtml_intro

  No clue how to debug javascrip other that write, compile, upload, refresh, guess, repeat

  I'm using class designators to set styles and id's for data updating
  for example:
  the CSS class .tabledata defines with the cell will look like
  <td><div class="tabledata" id = "switch"></div></td>

  the XML code will update the data where id = "switch"
  java script then uses getElementById
  document.getElementById("switch").innerHTML="Switch is OFF";


  .. now you can have the class define the look AND the class update the content, but you will then need
  a class for every data field that must be updated, here's what that will look like
  <td><div class="switch"></div></td>

  the XML code will update the data where class = "switch"
  java script then uses getElementsByClassName
  document.getElementsByClassName("switch")[0].style.color=text_color;


  the main general sections of a web page are the following and used here

  <html>
    <style>
    // dump CSS style stuff in here
    </style>
    <body>
      <header>
      // put header code for cute banners here
      </header>
      <main>
      // the buld of your web page contents
      </main>
      <footer>
      // put cute footer (c) 2021 xyz inc type thing
      </footer>
    </body>
    <script>
    // you java code between these tags
    </script>
  </html>


*/

// note R"KEYWORD( html page code )KEYWORD"; 
// again I hate strings, so char is it and this method let's us write naturally

const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en" class="js-focus-visible">

<title>Web Page Update Demo</title>

  <style>
    .bodytext {
      font-family: "Verdana", "Arial", sans-serif;
      font-size: 24px;
      text-align: left;
      font-weight: light;
      border-radius: 5px;
      display:inline;
    }
    .navbar {
      width: 100%;
      height: 40px;
      margin: 0;
      padding: 10px 0px;
      background-color: #FFF;
      color: #000000;
      border-bottom: 5px solid #293578;
    }
    .fixed-top {
      position: fixed;
      top: 0;
      right: 0;
      left: 0;
      z-index: 1030;
    }
    .navtitle {
      float: left;
      height: 50px;
      font-family: "Copperplate", "Arial", sans-serif;
      font-size: 35px; 
      font-weight: bold;
      line-height: 50px;
      padding-left: 75px;
    }
   .navheading {
     position: fixed;
     left: 60%;
     height: 50px;
     font-family: "Verdana", "Arial", sans-serif;
     font-size: 20px;
     font-weight: bold;
     line-height: 20px;
     padding-right: 20px;
   }
   .navdata {
      justify-content: flex-end;
      position: fixed;
      left: 70%;
      height: 50px;
      font-family: "Verdana", "Arial", sans-serif;
      font-size: 20px;
      font-weight: bold;
      line-height: 20px;
      padding-right: 20px;
   }
    .category {
      font-family: "Verdana", "Arial", sans-serif;
      font-weight: bold;
      font-size: 22px;   				/* Remote antenna control text */
      line-height: 50px;
      padding: 20px 10px 0px 10px;
      color: #000000;
    }
    .heading {
      font-family: "Verdana", "Arial", sans-serif;
      font-weight: normal;
      font-size: 28px;
      text-align: left;
    }
    .foot {
      font-family: "Verdana", "Arial", sans-serif;
      font-size: 20px;
      position: relative;
      height:   30px;
      text-align: center;   
      color: #0e0e0f;
      line-height: 20px;
    }
    .container {
      max-width: 1800px;
      margin: 0 auto;
    }
    .button {
        /* display: inline-block;
        background-color: #f5cba4d7;
        width: 20px;
        color: #000000;
        text-align: center; */
        font-weight: bold;
        border-radius: 5px
    }
    .biggerbutton {
        /* display: inline-block;
        background-color: #c5a9f1d7;
        color: #000000;
        text-align: center; */
        font-weight: bold;
        border-radius: 5px;
    }
    .emerbutton {
        /* display: inline-block;
        background-color: #f58ad5be;
        color: #000000;
        text-align: center; */
        font-weight: bold;
        border-radius: 5px;
    }
    /* input[type="radio"] {
      margin-left: 80;
      transform: scale(2);
      border-radius: 10px;
	  } */
  </style>

  <body style="background-color: #c9edf4" onload="process()">

    <header>
      <div class="navbar fixed-top">
          <div class="container">
            <div class="navtitle">RAS 3 WiFi</div>
            <div class="navdata" id = "date">mm/dd/yyyy</div>
            <div class="navheading">DATE</div><br>
            <div class="navdata" id = "time">00:00:00</div>
            <div class="navheading">TIME</div>
          </div>
      </div>
    </header>
   
    <br>
    <br>
    <div class="category">Remote Antenna Control</div>

  <!-- MY Radio buttons start here -->
	<span style="display:inline-block; width: 10px;"></span>
		<input type="button" class="button" id="R1Select" name="Radio1" value="Receivers" onclick="RadiosGrpButtonPress(this.id)">
	    <span style="display:inline-block; width: 20px;"></span>
		<input type="button" class="button" id="R2Select" name="Radio2" value="Flex-6600" onclick="RadiosGrpButtonPress(this.id)">
	<span style="display:inline-block; width: 20px;"></span>
		<input type="button" class="button" id="R3Select" name="Radio3" value="IC-7300" onclick="RadiosGrpButtonPress(this.id)">
	<span style="display:inline-block; width: 20px;"></span>
		<input type="button" class="button" id="R4Select" name="Radio4" value="IC-705" onclick="RadiosGrpButtonPress(this.id)">

	<span style="display:inline-block; width: 20px;"></span>
		<input type="button" class="biggerbutton" id="Aux1_OnSel" name="Aux1Grp" value="Astron Power On"  onclick="Aux1GrpButtonPress(this.id)">
	<span style="display:inline-block; width: 20px;"></span>
		<input type="button" class="biggerbutton" id="Aux2_OnSel" name="Aux2Grp" value="Flex Remote On"  onclick="Aux2GrpButtonPress(this.id)">
	  		<br>
	<span style="display:inline-block; width: 10px;"></span>
		<input type="button" class="emerbutton" id="ALL_GND" name="ALL_GroundedGrp" value="ALL GND" onclick="AllGndPress(this.id)"> 

<svg width="160" height="60" >
    <circle id="Xmit_Ind"  cx="75" cy="38" r="15" fill="red" stroke="black" stroke-width="2"/>
  <text x="75" y="42" font-size="10" text-anchor="middle" fill="black">XMT</text>
</svg>

  <span style="display:inline-block; width:150px;"></span>	
		<input type="button" class="biggerbutton" id="Aux1_OffSel" name="Aux1_Off" value="Astron Power Off" onclick="Aux1GrpButtonPress(this.id)">
	<span class="biggerbutton" style="display:inline-block; width: 20px;"></span>
		<input type="button" class="biggerbutton" id="Aux2_OffSel" name="Aux2_Off" value="Flex Remote Off" onclick="Aux2GrpButtonPress(this.id)"><br>
    <br>
    <span style="display:inline-block; width: 30px;"></span>	
  <br>
  <textarea id="RAS3TB" name="w3review" rows="10" cols="85"></textarea>
        <!-- MY buttons ends here -->
 <footer div class="foot" id = "Chincey" >WA9FVP RAS3 Web Page </div></footer>

  </body>

  <script type = "text/javascript">

    // global variable visible to all java functions
    var xmlHttp=createXmlHttpObject();

    // helper function to create XML object, returns that created object
    function createXmlHttpObject(){
      if(window.XMLHttpRequest){
        xmlHttp=new XMLHttpRequest();
      }
      else{
        xmlHttp=new ActiveXObject("Microsoft.XMLHTTP");
      }
      return xmlHttp;
    }

    //  handle a selection made of the Radios button group for the 4 radios
    function RadiosGrpButtonPress(value) {
      // passing in the value of the object clicked/selected is one way... but...
      console.log("Radio " + value);
      //TextLog(value);
            
      // turn the button yellow indicating a request was made of the server.
      // the next hardware update should change this appropriately
      document.getElementById(value).style.backgroundColor="#ffff00";

      // another is to just query which one has been checked in the radio group
      //console.log(">>>Selected value for RadiosGroup: " + document.querySelector("input[name=RadioGrp]:checked").value); // rpb
      //document.getElementById('RAS3TB').value = value;
      /*
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("button1").innerHTML = this.responseText;
        }
      }
      */
      var xhttp = new XMLHttpRequest(); 
      xhttp.open("PUT", value, true); // send name of radio button selected to server
      xhttp.send();
    }

    // handler for a selection made of one of the two buttons in the Aux1 button group
    function Aux1GrpButtonPress(value) {
      console.log("Aux1 " + value);
      //TextLog(value);
      
      // turn the button yellow indicating a request was made of the server.
      // the next hardware update should change this appropriately
      document.getElementById(value).style.backgroundColor="#ffff00";
      var xhttp = new XMLHttpRequest(); 
      xhttp.open("PUT", value, true); // send name of Aux1 button selected to server
      xhttp.send();
    }
    
    // handler for a selection made of one of the two buttons in the Aux2 button group
    function Aux2GrpButtonPress(value) {
      console.log("Aux2 " + value);
      //TextLog(value);

      // turn the button yellow indicating a request was made of the server.
      // the next hardware update should change this appropriately
      document.getElementById(value).style.backgroundColor="#ffff00";
      var xhttp = new XMLHttpRequest(); 
      xhttp.open("PUT", value, true); // send name of Aux2 button selected to server
      xhttp.send();
    }

    function AllGndPress(value) {
      console.log("AllGndPress " + value );
      //TextLog(value);
      document.getElementById("ALL_GND").style.backgroundColor="#ffff00";

      var xhttp = new XMLHttpRequest(); 
      xhttp.open("PUT", value, true); // send name of ALL Grounded button selected to server
      xhttp.send();
    }

    // try to emulate an autoscrolling log using a text area
    // call this function if you want to post something to the textarea on the webpage
    function TextLog(value) {
      var dt = new Date();
      var ta = document.getElementById("RAS3TB");
      ta.value += dt.toLocaleTimeString() + "> " + value + "\n";
      ta.scrollTop = ta.scrollHeight;
    }

    // function to handle the response FROM the ESP
    function response(){
      var message;
      var barwidth;
      var currentsensor;
      var xmlResponse;
      var xmldoc;
      var dt = new Date();
      var color = "#e8e8e8";
     
      // get the xml stream
      xmlResponse=xmlHttp.responseXML;

      // TODO:  WHY so many null responses ? 
      if(xmlResponse == null)  return;

      // get host date and time
      document.getElementById("time").innerHTML = dt.toLocaleTimeString();
      document.getElementById("date").innerHTML = dt.toLocaleDateString();

      // handle the case where INFO tag is null/undefined, its the only tag which may be so
      xmldoc = xmlResponse.getElementsByTagName("INFO") ;
      try {
        message = xmldoc[0].firstChild.nodeValue;
        TextLog(message);
        console.log(message);
      } catch (e) {
        ;//console.log("Catch on reading INFO");
      }

console.log((new XMLSerializer()).serializeToString(xmlResponse));  // debug, remove if desired

      xmldoc = xmlResponse.getElementsByTagName("R1");
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("R1Select").style.backgroundColor=color; 
      //console.log(message);

      xmldoc = xmlResponse.getElementsByTagName("R2"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("R2Select").style.backgroundColor=color; 
      //console.log(message);

      xmldoc = xmlResponse.getElementsByTagName("R3"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("R3Select").style.backgroundColor=color; 
      //console.log(message);

      xmldoc = xmlResponse.getElementsByTagName("R4"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("R4Select").style.backgroundColor=color; 
      //console.log(message);

      xmldoc = xmlResponse.getElementsByTagName("A1_On"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("Aux1_OnSel").style.backgroundColor=color; 
      //console.log(message);

      xmldoc = xmlResponse.getElementsByTagName("A1_Off"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("Aux1_OffSel").style.backgroundColor=color; 
      //console.log(message);

      xmldoc = xmlResponse.getElementsByTagName("A2_On"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("Aux2_OnSel").style.backgroundColor=color; 
      
      xmldoc = xmlResponse.getElementsByTagName("A2_Off"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#00ff00" : color = "#ffffff";  // either selected green, or white
      document.getElementById("Aux2_OffSel").style.backgroundColor=color; 
      //console.log(message);
    
      xmldoc = xmlResponse.getElementsByTagName("ALL_GND"); 
      message = xmldoc[0].firstChild.nodeValue;
      message == "1" ? color="#ffcce6" : color = "#ffffff";  // either selected green, or white
      document.getElementById("ALL_GND").style.backgroundColor=color; 
      //console.log(message);

xmldoc = xmlResponse.getElementsByTagName("Xmit_Ind"); 
message = xmldoc[0].firstChild.nodeValue;
message == "1" ? color="#ff0000" : color = "#ffffff";  // either selected red, or white
document.getElementById("Xmit_Ind").style.fill=color; 
//console.log(message);
    
    }
    
    // general processing code for the web page to ask for an XML steam
    // this is actually why you need to keep sending data to the page to 
    // force this call with stuff like this
    // server.on("/xml", SendXML);
    // otherwise the page will not request XML from the ESP, and updates will not happen
    function process(){
      // var xhttp = new XMLHttpRequest();
      // if(xhttp.readyState == 0 || xhttp.readyState == 4) {
      //     xhttp.open("GET","xml",true); 
      //     xhttp.onreadystatechange = response;
      //     xhttp.send(null);
      // }

      if(xmlHttp.readyState == 0 || xmlHttp.readyState == 4) {
          xmlHttp.open("GET","xml",true);
          xmlHttp.onreadystatechange = response;
          xmlHttp.send(null);
      }       
        // you may have to play with this value, big pages need more porcessing time, and hence
        // a longer timeout
      setTimeout("process()",1000); // this defines the polling interval/update freq in ms
    }
  </script>

</html>
)=====";
