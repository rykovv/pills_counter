#include "Arduino.h"

const uint8_t index_html_l[] = R"=====(<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Pills Counter</title>
    <style>
      * {
        box-sizing: border-box;
      }
      body {
        font-family: Arial, Helvetica, sans-serif;
      }
      header {
        padding: 30px;
        text-align: center;
        font-size: 35px;
        color: white;
      } 
      header h1 {
        color: black;
      }
      nav {
        float: left;
        width: 30%;
        padding: 20px;
      }
      nav ul {
        list-style-type: none;
        padding: 0;
      }
      article {
        float: left;
        padding: 20px;
        width: 70%;
      }
      section::after {
        content: "";
        display: table;
        clear: both;
      }
      footer {
        padding: 10px;
        text-align: center;
        color: white;
      }
      footer p {
        color: black;
      }
      @media (max-width: 600px) {
        nav, article {
          width: 100%;
          height: auto;
        }
      }
      </style>
  </head>
  <body>
    <header>
      <h1>Counter: <span id="pills_ctr"></span></h1>
    </header>
    <section>
      <nav>
        <ul>
          <li><button>start</button> counting</li>
          <li><button>reset</button> counter</li>
          <li>Counting algorithm:<br><select>
            <option>grid counting</option>
            <option>1 line detection</option>
            <option>2 lines detection</option>
            <option>3 lines detection</option>
            </select></li>
          <li><button>start</button> visual monitor</li>
          <li><button>set</button> alarm</li>
          <li>Alarm Callback Link:<input type="text"></li>
        </ul>
      </nav>
      
      <article>
        <figure>
          <div id="stream-container" class="image-container">
            <img id="stream" src="http://2.2.2.1:81/counter_stream">
          </div>
        </figure>
      </article>
    </section>

    <footer>
      <p>Pills Counter. EEE221 Term Project. Vladislav Rykov</p>
    </footer>
  </body>

  <script>
  document.addEventListener('DOMContentLoaded', function (event) {
    var pills_view = document.getElementById("pills_ctr");
    (function worker() {
      fetch('/pills').then(resp => resp.text()).then(text => {
        pills_view.innerHTML = text;
      });
      setTimeout(worker, 1000);
	  })();
  })
  </script>
</html>)=====";

const uint8_t index_html[] = R"=====(<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>ESP32 Pills Counter</title>
  </head>
  <body>
    <section class="main">
      <div id="content">
        <center>
          <h1>Counter: <span id="pills_ctr"></span></h1>

          <figure>
            <div id="stream-container" class="image-container">
              <img id="stream" src="http://2.2.2.1:81/counter">
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
      fetch('/pills').then(resp => resp.text()).then(text => {
        pills_view.innerHTML = text;
      });
      setTimeout(worker, 1000);
	  })();
  })
  </script>
</html>)=====";
size_t index_html_len = sizeof(index_html)-1;