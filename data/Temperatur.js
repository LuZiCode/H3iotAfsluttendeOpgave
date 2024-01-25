var chartT;

// Get current sensor readings when the page loads
window.addEventListener('load', function () {
  setEventListeners();

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
    },
    timezoneOffset: new Date().getTimezoneOffset() + 60
  });

  var socket = new WebSocket('ws://' + window.location.hostname + '/ws');

  socket.binaryType = 'arraybuffer'; // Set binary type to 'arraybuffer'

  socket.onmessage = function (event) {
    if (event.data instanceof ArrayBuffer) {
      console.log("Binary Data Received");

      // Handle binary data (ArrayBuffer) here
      var byteArray = new Uint8Array(event.data);

      // Convert byteArray to numeric values or handle binary data as needed
      var temperature1 = byteArray[0]; // Example: use the first byte as a numeric value
      var temperature2 = byteArray[1]; // Example: use the second byte as a numeric value

      // Update the chart with new data
      var timestamp = new Date().getTime();

      // Use the numeric values for Temperature #1 and Temperature #2 series
      chartT.series[0].addPoint([timestamp, temperature1], true, false, true);
      chartT.series[1].addPoint([timestamp, temperature2], true, false, true);

      console.log("Real-time update:", timestamp, temperature1, temperature2);
    } else {
      console.log("Text Data Received");

      // Handle text data (JSON)
      try {
        var data = JSON.parse(event.data);

        // Update the chart with new data
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

  socket.onopen = function (event) {
    console.log('WebSocket connection opened:', event);
  };

  socket.onclose = function (event) {
    console.log('WebSocket connection closed:', event);
    // Attempt to reconnect after a delay (e.g., 5 seconds)
    setTimeout(function () {
      // Reconnect
      socket = new WebSocket('ws://' + window.location.hostname + '/ws');
      socket.binaryType = 'arraybuffer';
      // Add event listeners (onmessage, onopen, onclose, onerror)
      socket.onopen = function (event) {
        console.log('WebSocket connection opened:', event);
      };
      socket.onmessage = function (event) {
        // Your message handling logic here
      };
      socket.onclose = function (event) {
        console.log('WebSocket connection closed:', event);
      };
      socket.onerror = function (error) {
        console.error('WebSocket error:', error);
        console.log('WebSocket readyState:', socket.readyState);
      };
    }, 5000);
  };

// Get historical data from the server
fetch(Site_address + '/loaddata')
  .then(function (response) {
    console.log("RESPONSE!!!!", response);
    // Check if the response status is OK (200)
    if (response.ok) {
      console.log("RESPONSE TEXT!!!!", response.text);
      return response.text(); // Read the CSV data as text
    } else {
      throw new Error('Failed to load historical data');
    }
  })
  .then(function (csvData) {
    console.log("CSV DATA!!!!", csvData);
    // Parse the CSV data using papaparse
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

  function setEventListeners() {
    var pressed = false;
    const burgerBtn = this.document.getElementById('burgerBtn');
    const brugerContent = this.document.getElementById('burgerContent');
    const burgerGrafBtns = this.document.getElementsByClassName('burger-graf-btn')

    burgerBtn.addEventListener('click', function () {
      if (!pressed) {
        pressed = true;
        brugerContent.classList.add('burger-show');
      } else {
        pressed = false;
        brugerContent.classList.remove('burger-show');
      }
    })

    for (var i = 0; i < burgerGrafBtns.length; i++) {
      burgerGrafBtns[i].addEventListener('click', function (event) {
        initBtnMethod(event.target.value);
      })
    }
  }

  function initBtnMethod(method) {
    if (method == "cleargraf") {
      clearChart();
    }
    else {
      fetch(`/buttonHandling?param=${encodeURIComponent(method)}`)
        .then(response => response.text())
        .then(data => {
          var responseElement = document.getElementById('response-message');
          responseElement.classList.add('success'); 
          responseElement.textContent = data;
        })
        .catch(error => {
          console.error('Der opstod en fejl:', error);
          var responseElement = document.getElementById('response-message');
          responseElement.classList.add('fail');
          responseElement.textContent = error;
        });
    }
  }
  function clearChart() {
    chartT.series[0].setData([], true);
    chartT.series[1].setData([], true);
    var responseElement = document.getElementById('response-message');
    responseElement.classList.add('success'); 
    responseElement.textContent = "Grafen er clearet";
  }  
});
