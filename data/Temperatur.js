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

// Get historical data from the server
fetch(Site_address + 'loaddata')
  .then(function (response) {
    // Check if the response status is OK (200)
    if (response.ok) {
      return response.text(); // Read the CSV data as text
    } else {
      throw new Error('Failed to load historical data');
    }
  })
  .then(function (csvData) {
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
            var timestamp = new Date(dateTimeString).getTime();

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
        });
      },
    });
  })
  .catch(function (error) {
    console.error('Fetch error:', error);
  });
