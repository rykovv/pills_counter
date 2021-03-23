#include "Arduino.h"

const uint8_t index_html[] = R"=====(<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>ESP32 Pills Counter</title>
    <script type="text/javascript" srs="http://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js">
  </head>
  <body>
    <section class="main">
      <div id="content">
        <center>
          <h3> Pills found: </h3>
          <h3 id="pills_ctr"></h3>

          <figure>
            <div id="stream-container" class="image-container">
              <img id="stream" src="/stream">
            </div>
          </figure>
        </center>
      </div>
    </section>
  </body>

  <script>
  document.addEventListener('DOMContentLoaded', function (event) {
    var pills_view = document.getElementById("pills_ctr");
    (function worker() {
      $.get('http://2.2.2.1:81/pills', function(response) {
        pills_view.html(response.data);
        console.log(response.data);
      });
      setTimeout(worker, 1000);
	  })();
  })
  </script>
</html>)=====";
size_t index_html_len = sizeof(index_html)-1;