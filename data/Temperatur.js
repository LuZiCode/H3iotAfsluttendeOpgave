var chartT;

// Get current sensor readings when the page loads
window.addEventListener('load', function () {
  setEventlisterners();
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

function setEventlisterners() {
  var pressed = false;
  const burgerBtn = this.document.getElementById('burgerBtn');
  const brugerContent = this.document.getElementById('burgerContent');
  const burgerGrafBtns = this.document.getElementsByClassName('burger-graf-btn')
  burgerBtn.addEventListener('click', function() {
    if (!pressed) {
      pressed = true;
      brugerContent.classList.add('burger-show');
    }
    else {
      pressed = false;
      brugerContent.classList.remove('burger-show');
    }
  })
  for (var i = 0; i < burgerGrafBtns.length; i++) {
    burgerGrafBtns[i].addEventListener('click', function(event) {
      initBtnMethod(event.target.value);
    })
  }
}

function initBtnMethod(method) {
  fetch(`/buttonHandling?param=${encodeURIComponent(method)}`)
    .then(response => response.text())
    .then(data => console.log(data))
    .catch(error => console.error('Der opstod en fejl:', error));
}

// Plot temperature in the temperature chart
function plotTemperature(jsonValue) {
  if (jsonValue.hasOwnProperty("time")) {
    // var x = new Date(jsonValue.time).getTime();
    // console.log(x);
    
    // Assuming the JSON structure is like {"sensor1": "...", "sensor2": "...", "time": "..."}
    Object.keys(jsonValue).forEach((key, i) => {
      var x = jsonValue.time;
      if (key !== "time") {
        var y = Number(jsonValue[key]);
        console.log("CHART SERIES!!!!", chartT.series[i])
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
