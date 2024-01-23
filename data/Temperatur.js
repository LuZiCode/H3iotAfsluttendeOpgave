var chartT;

// Get current sensor readings when the page loads
window.addEventListener('load', function () {
  // Create Temperature Chart
  chartT = new Highcharts.Chart({
    chart: {
      renderTo: 'chart-temperature'
    },
    series: [
      {
        name: 'Temperature #1',
        type: 'line',
        color: '#101D42',
        marker: {
          symbol: 'circle',
          radius: 3,
          fillColor: '#101D42',
        }
      },
      {
        name: 'Temperature #2',
        type: 'line',
        color: '#00A6A6',
        marker: {
          symbol: 'square',
          radius: 3,
          fillColor: '#00A6A6',
        }
      }
    ],
    title: {
      text: undefined
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: {
        text: 'Temperature Celsius Degrees'
      }
    },
    credits: {
      enabled: false
    }
  });

  // Fetch and plot initial readings
  getReadings();
});

// Plot temperature in the temperature chart
function plotTemperature(jsonValue) {
  if (jsonValue.hasOwnProperty("time")) {
    var x = new Date(jsonValue.time).getTime();
    console.log(x);

    // Assuming the JSON structure is like {"sensor1": "...", "sensor2": "...", "time": "..."}
    Object.keys(jsonValue).forEach((key, i) => {
      if (key !== "time") {
        var y = Number(jsonValue[key]);

        if (chartT.series[i].data.length >= 30) {
          chartT.series[i].addPoint([x, y], true, true, true);
        } else {
          chartT.series[i].addPoint([x, y], true, false, true);
        }
      }
    });
  }
}

// Function to get current readings on the webpage when it loads for the first time
function getReadings() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      plotTemperature(myObj);
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();
}

// WebSocket handling
var socket = new WebSocket("ws://" + window.location.hostname + ":80/ws");

socket.onopen = function (event) {
  console.log("WebSocket connected");
};

socket.onmessage = function (event) {
  console.log("WebSocket message", event.data);
  var myObj = JSON.parse(event.data);
  console.log(myObj);
  plotTemperature(myObj);
};

socket.onclose = function (event) {
  console.log("WebSocket disconnected");
};
