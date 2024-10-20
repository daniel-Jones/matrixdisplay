const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <meta content="text/html;charset=utf-8" http-equiv="Content-Type">
  <meta name="author" content="Daniel Jones">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Matrix Display</title>
  <style>
    .block {
      display: block;
      width: 100%;
      border: none;
      background-color: #04AA6D;
      color: white;
      padding: 14px 28px;
      font-size: 16px;
      cursor: pointer;
      text-align: center;
    }

    .block:hover {
      background-color: #ddd;
      color: black;
    }

    body {
      background: #F7F0FF;
    }

    .center {
      display: flex;
      justify-content: center;
      align-items: center;
      height: auto;
    }

    .content {
      border-radius: 10px;
      //border: 3px solid green;
      padding-top: 10px;
      padding-right: 10px;
      padding-bottom: 10px;
      padding-left: 10px;
      max-width: 95%;
      max-height: 95%;
    }

    #centertext {
      text-align: center
    }

    #footer {
      text-align: left;
      position: fixed;
      bottom: 0;
      left: 0
    }

    fieldset {
      margin-bottom: 1em;
      border-right: 1px solid #666;
      border-bottom: 3px solid #000000;
      border: 3px solid #000000;
    }

    table.minimalistBlack {
      border: 3px solid #000000;
      width: 100%;
      text-align: center;
      border-collapse: collapse;
    }

    table.minimalistBlack td,
    table.minimalistBlack th {
      border: 1px solid #000000;
      padding: 5px 4px;
    }

    table.minimalistBlack tbody td {
      font-size: 13px;
    }

    table.minimalistBlack thead {
      background: #CFCFCF;
      background: -moz-linear-gradient(top, #dbdbdb 0%, #d3d3d3 66%, #CFCFCF 100%);
      background: -webkit-linear-gradient(top, #dbdbdb 0%, #d3d3d3 66%, #CFCFCF 100%);
      background: linear-gradient(to bottom, #dbdbdb 0%, #d3d3d3 66%, #CFCFCF 100%);
      border-bottom: 3px solid #000000;
    }

    table.minimalistBlack thead th {
      font-size: 15px;
      font-weight: bold;
      color: #000000;
      text-align: center;
    }

    table.minimalistBlack tfoot td {
      font-size: 14px;
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 25px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }

    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 25px;
      height: 25px;
      background: #04AA6D;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 25px;
      height: 25px;
      background: #04AA6D;
      cursor: pointer;
    }

    select {
      width: auto;
      padding: 5px 5px;
      border: none;
      border-radius: 4px;
      background-color: #f1f1f1;
    }
  </style>
</head>

<body>

  <div class="center">
    <div class="content">
      <h1 id="centertext">Matrix Display</h1>

      <table class="minimalistBlack">
        <thead>
          <tr>
            <th>Special string</th>
            <th>Replacement</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>%date%</td>
            <td>The selected date (dd/mm/yy)</td>
          </tr>
          <tr>
            <td>%shortdate%</td>
            <td>The selected date short (20 Jan)</td>
          </tr>
          <tr>
            <td>%time12% or %time12s%</td>
            <td>time in 12 hour format HH:MM or HH:MM:SS</td>
          </tr>
          <tr>
            <td>%time24% or %time24s%</td>
            <td>time in 24 hour format HH:MM or HH:MM:SS</td>
          </tr>

          <!--
  <tr>
	<td>%weeks%</td>
    <td>Weeks until the selected date</td>
</tr>
  <tr>
    <td>%days%</td>
    <td>Days until the selected date</td>
</tr>
  <tr>
	<td>%hours%</td>
    <td>Hours until the selected date</td>
</tr>

  <tr>
    <td>%minutes%</td>
    <td>Minutes until the selected date</td>
</tr>
  <tr>
	<td>%seconds%</td>
    <td>Seconds until the selected date</td>
</tr>
-->
          </tr>
        </tbody>
      </table>     
      <form id="saveform">
        <fieldset>
          <legend>Customisation:</legend>
          <button type="button" id="toggledisplay" onclick="sendPostRequest('matrix', 'toggle')">Toggle Display</button>&nbsp;
          <button type="button" id="restart" onclick="sendPostRequest('restart', '')">Restart Sign</button><br>
          Wireless (required for time sync):<br>
          <input type="text" id="cssid" name="cssid" placeholder="ssid"> &nbsp; <input type="password" id="cpassword"
            name="cpassword" placeholder="password">
          <p id="wifistatus">No data</p>
          <label for="start">Countdown date:</label>

          <input type="date" id="datep" name="datep"><br>
          <label for="brightness">Brightness:</label>
          <input type="range" min="0" max="15" value="3" class="slider" name="brightness" id="brightness">
          <label for="autoBrightness">Auto Brightness:</label>
          <input type="checkbox" id="autoBrightness" name="autoBrightness" value="autoBrightness">
          <label for="autoBrightnessThreshold">Maximum (0-512):</label>
          <input type="number" id="autoBrightnessThreshold" name="autoBrightnessThreshold" min="0" max="512" value="400"><br><br>
          <input class="block" type="submit" value="Save">
          <br>
          <div style="overflow-y: scroll; height:400px;">
            <script>
              var html = `
            <input type="text" id="m%d" name="m%d" placeholder="Message %d"> &nbsp; 
            <input type="checkbox" id="m%denabled" name="m%denabled" value="Enabled">
            <label for="m%dinvert">Enabled</label>&nbsp;<br>
            <label for="m%deffect2">Start:</label> &nbsp;
            <select name="m%deffect" id="m%deffect">
              <option selected="selected" value="4">Scroll right to left</option>
              <option value="5">Scroll left to right</option>
              <option value="0">No effect</option>
              <option value="1">Static</option>
              <option value="2">Scroll up</option>
              <option value="3">Scroll down</option>
              <option value="7">Slice</option>
              <option value="8">Mesh</option>
              <option value="9">Fade</option>
              <option value="10">Dissolve</option>
              <option value="11">Blinds</option>
              <option value="12">Random</option>
              <option value="13">Wipe</option>
              <option value="14">Wipe (with bar)</option>
              <option value="19">Curtains (from center)</option>
              <option value="20">Curtains (c/with bar)</option>
              <option value="21">Curtains (from ends)</option>
              <option value="22">Curtains (e/with bar)</option>
              <option value="23">Scroll up+left</option>
              <option value="24">Scroll up+right</option>
              <option value="25">Scroll down+left</option>
              <option value="26">Scroll down+right</option>
              <option value="27">Grow up</option>
              <option value="28">Grow down</option>
            </select> 
            <label for="m%deffect2">End:</label> &nbsp;
	    <select name="m%deffect2" id="m%deffect2">
              <option selected="selected" value="4">Scroll right to left</option>
              <option value="5">Scroll left to right</option>
              <option value="0">No effect</option>
              <option value="1">Static</option>
              <option value="2">Scroll up</option>
              <option value="3">Scroll down</option>
              <option value="7">Slice</option>
              <option value="8">Mesh</option>
              <option value="9">Fade</option>
              <option value="10">Dissolve</option>
              <option value="11">Blinds</option>
              <option value="12">Random</option>
              <option value="13">Wipe</option>
              <option value="14">Wipe (with bar)</option>
              <option value="19">Curtains (from center)</option>
              <option value="20">Curtains (c/with bar)</option>
              <option value="21">Curtains (from ends)</option>
              <option value="22">Curtains (e/with bar)</option>
              <option value="23">Scroll up+left</option>
              <option value="24">Scroll up+right</option>
              <option value="25">Scroll down+left</option>
              <option value="26">Scroll down+right</option>
              <option value="27">Grow up</option>
              <option value="28">Grow down</option>
            </select> <br>
            <label for="m%dspeed">Speed:</label>&nbsp;
      <select name="m%dspeed" id="m%dspeed">
              <option value="slow">Slow</option>
              <option selected="selected" value="medium">Medium</option>
              <option value="fast">Fast</option>
            </select> &nbsp;

	    <label for="m%dpause">Pause in seconds:</label>
	    <input type="number" style="width: 30px;" id="m%dpause" name="m%dpause" min="0" max="34600">


            <input type="checkbox" id="m%dinvert" name="m%dinvert" value="Invert">
	                <label for="m%dinvert">Invert</label>
		<hr>
	`;
              for (i = 0; i < 20; i++) {
                document.write(html.replaceAll('%d', i + 1));
              }
            </script>
          </div><br>

          <input class="block" type="submit" value="Save">
        </fieldset>
      </form>
    </div>
  </div>
  <script>
    const ajax = async (config) => {
      const request = await fetch(config.url, {
        method: config.method,
        headers: {
          'Accept': 'application/json',
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(config.payload)
      });
      response = await request.json();
      console.log('response', response)
      return response
    }

    function handleSubmit(event) {
      event.preventDefault();
      const data = new FormData(event.target);
      const value = Object.fromEntries(data.entries());
      console.log({ value });
      response = ajax({
        method: 'POST',
        url: '/msg',
        payload: value
      })
    }

    var getJSON = function (url, callback) {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', url, true);
      xhr.responseType = 'json';
      xhr.onload = function () {
        var status = xhr.status;
        if (status === 200) {
          callback(null, xhr.response);
        } else {
          callback(status, xhr.response);
        }
      };
      xhr.send();
    };



    window.addEventListener("load", (event) => {
      console.log("running loadconfig");
      getJSON('/config',
        function (err, data) {
          if (err !== null) {
            alert('Something went wrong: ' + err);
          } else {
            console.log({ data });
            /* 
             switch (data.brightness)
             {
               case 3:
               console.log("brightness low");
               document.getElementById("Low").checked = true;
               break;
               case 7:
               console.log("brightness medium");
               document.getElementById("Medium").checked = true;
               break;
               case 15:
               console.log("brightness high");
               document.getElementById("High").checked = true;
               break;
               default:
               console.log("Default brightness");
               document.getElementById("Medium").checked = true;
               break;
             }
           */

            document.getElementById("brightness").value = data.brightness;
            document.getElementById("autoBrightness").checked = data.autoBrightness;
            document.getElementById("autoBrightnessThreshold").value = data.autoBrightnessThreshold;
            document.getElementById("cssid").value = data["cssid"];
            document.getElementById("cpassword").value = data["cpassword"];
            document.getElementById("datep").valueAsDate = new Date(data["datep"]);
            if (data["wifistatus"] == "Not connected") {
              document.getElementById("wifistatus").innerText = "Not connected to a network";
            }
            else {
              document.getElementById("wifistatus").innerText = "Access this page through the network above: http://" + data["wifistatus"];
            }
            // 1
            for (i = 0; i < 20; i++) {
              n = i + 1;
              document.getElementById("m" + n).value = data["m" + n];
              document.getElementById("m" + n + "enabled").checked = data["m" + n + "enabled"];
              /*
              switch (data["m"+n+"effect"])
              {case 5:document.getElementById("m"+n+"effect").value = "ltr";break;default:case 4:document.getElementById("m"+n+"effect").value = "rtl";break;}
              */
              switch (data["m" + n + "speed"]) { case 60: document.getElementById("m" + n + "speed").value = "slow"; break; default: case 40: document.getElementById("m" + n + "speed").value = "medium"; break; case 20: document.getElementById("m" + n + "speed").value = "fast"; break; }
              document.getElementById("m" + n + "pause").value = data["m" + n + "pause"];
              document.getElementById("m" + n + "invert").checked = data["m" + n + "invert"];

              document.getElementById("m" + n + "effect").value = data["m" + n + "effect"];
              document.getElementById("m" + n + "effect2").value = data["m" + n + "effect2"];
            }


          }
        });
    });

    const form = document.getElementById('saveform');
    form.addEventListener('submit', handleSubmit);

    document.getElementById("datep").valueAsDate = new Date();


    function sendPostRequest(key, value) {
    const data = {
        [key]: value
    };

    fetch('/cmd', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.json();
    })
    .then(data => {
        console.log('Success:', data);
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}

  </script>
</body>

</html>
)rawliteral";
