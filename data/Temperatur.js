var chartT;

// Event listener for window load
window.addEventListener('load', function () {
  setEventListeners();

  // Initialize the Highcharts temperature chart
  chartT = new Highcharts.Chart({
    chart: {
      renderTo: 'chart-temperature' // Chart container ID
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
      text: undefined // No title for the chart
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
    },
    timezoneOffset: new Date().getTimezoneOffset() + 60 // Adjust timezone
  });

  console.log("Temperature chart initialized");

  // Establish WebSocket connection
  var socket = new WebSocket('ws://' + window.location.hostname + '/ws');
  socket.binaryType = 'arraybuffer'; // Set binary data type for WebSocket

  // WebSocket message event handler
  socket.onmessage = function (event) {
    if (event.data instanceof ArrayBuffer) {
      console.log("Binary Data Received");
      var byteArray = new Uint8Array(event.data);
      var temperature1 = byteArray[0];
      var temperature2 = byteArray[1];
      var timestamp = new Date().getTime();
      chartT.series[0].addPoint([timestamp, temperature1], true, false, true);
      chartT.series[1].addPoint([timestamp, temperature2], true, false, true);
    } else {
      console.log("Text Data Received");
      try {
        var data = JSON.parse(event.data);
        var dateTimeString = new Date().toDateString() + ' ' + data.time;
        var timestamp = new Date(dateTimeString + " UTC").getTime();
        var temperature1 = Number(data.sensor1);
        var temperature2 = Number(data.sensor2);
        chartT.series[0].addPoint([timestamp, temperature1], true, false, true);
        chartT.series[1].addPoint([timestamp, temperature2], true, false, true);

        console.log("Real-time update:", timestamp, temperature1, temperature2);
      } catch (error) {
        console.error('Error parsing JSON:', error);
      }
    }
  };

  // WebSocket open event handler
  socket.onopen = function (event) {
    console.log('WebSocket connection opened:', event);
  };

  // WebSocket close event handler
  socket.onclose = function (event) {
    console.log('WebSocket connection closed:', event);
    setTimeout(function () {
      socket = new WebSocket('ws://' + window.location.hostname + '/ws');
      socket.binaryType = 'arraybuffer';
      socket.onopen = function (event) {
        console.log('WebSocket reconnected:', event);
      };
      
      socket.onerror = function (error) {
        console.error('WebSocket error:', error);
        console.log('WebSocket readyState:', socket.readyState);
      };
    }, 5000);
  };

  // Fetch historical data from the server
  fetch(Site_address + '/loaddata')
    .then(function (response) {
      console.log("Fetch response received", response);
      if (response.ok) {
        return response.text();
      } else {
        throw new Error('Failed to load historical data');
      }
    })
    .then(function (csvData) {
      console.log("CSV data received", csvData);
      // CSV data parsing logic
      Papa.parse(csvData, {
        header: false, // Assumes the first row contains headers
        dynamicTyping: false, // Automatically convert values to numbers if possible
        complete: function (results) {
          if (results.errors.length > 0) {
            // Log any parsing errors
            console.error("CSV Parsing Errors:", results.errors);
          }

          var historicalData = results.data;

          // Extract the last 30 temperature values and their timestamps
          var last30Data = historicalData.slice(-30);

          // Plot the last 30 temperature values
          last30Data.forEach(function (item) {
            if (item.length === 5 && item[1] && item[2] && item[3] && item[4]) {
              // Combine the date and time to create a complete timestamp
              var dateTimeString = item[1] + " " + item[2];
              var timestamp = new Date(dateTimeString + " UTC").getTime();

              // Check if the row contains valid temperature values
              var isValidRow = !isNaN(timestamp) && !isNaN(item[3]) && !isNaN(item[4]);
              if (isValidRow) {
                // Use the temperature value for Temperature #1 series
                var temperature1 = Number(item[3]);
                chartT.series[0].addPoint([timestamp, temperature1], true, false, true);

                // Use the temperature value for Temperature #2 series
                var temperature2 = Number(item[4]);
                chartT.series[1].addPoint([timestamp, temperature2], true, false, true);

                console.log("Processed historical data:", timestamp, temperature1, temperature2);
              } else {
                console.error("Invalid historical data row:", item);
              }
            } else {
              console.error("Invalid historical data row:", item);
            }
          });
        },
      });
    })
    .catch(function (error) {
      console.error('Fetch error:', error);
    });

  // Set up event listeners for UI elements
  function setEventListeners() {
    var pressed = false;
    const burgerBtn = document.getElementById('burgerBtn');
    const brugerContent = document.getElementById('burgerContent');
    const burgerGrafBtns = document.getElementsByClassName('burger-graf-btn')

    burgerBtn.addEventListener('click', function () {
      pressed = !pressed;
      brugerContent.classList.toggle('burger-show', pressed);
    });

    for (var i = 0; i < burgerGrafBtns.length; i++) {
      burgerGrafBtns[i].addEventListener('click', function (event) {
        initBtnMethod(event.target.value);
      });
    }
  }

  // Function to handle button methods
  function initBtnMethod(method) {
    if (method == "cleargraf") {
      clearChart();
    } else {
      fetch(`/buttonHandling?param=${encodeURIComponent(method)}`)
        .then(response => response.text())
        .then(data => {
          var responseElement = document.getElementById('response-message');
          responseElement.classList.add('success'); 
          responseElement.textContent = data;
        })
        .catch(error => {
          console.error('Error in button handling:', error);
          var responseElement = document.getElementById('response-message');
          responseElement.classList.add('fail');
          responseElement.textContent = error;
        });
    }
  }

  // Function to clear the chart
  function clearChart() {
    chartT.series[0].setData([], true);
    chartT.series[1].setData([], true);
    var responseElement = document.getElementById('response-message');
    responseElement.classList.add('success'); 
    responseElement.textContent = "Chart cleared";
  }  
});
