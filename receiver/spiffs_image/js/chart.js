/**
* 
*/
var rainData;
var infoData = 0; // for info, voltages etc
var rawWeightData = 0; // debug raw weight for loadcell

var chartRdy = false;
var tick = 0;
var dontDraw = false;
var halt = false;
var chartHeigth = 500;
var simValue1 = 0;
var simValue2 = 0;
var table;
var presc = 1;
var simMssgCnts = 0;

var missingLoraMessgages = 0;

var GRAMSPERMM = 13.2;
var MINUTESPERTICK = 15;// log interval 
var LOGDAYS = 7;
var MAXPOINTS = LOGDAYS * 24 * 60 / MINUTESPERTICK;

var SIMULATE = false;

const varsEnum = { "day0": 0, "day1": 1, "day2": 2, "day3": 3, "day4": 4, "day5": 5, "day6": 6 };
var displayNames = ["", "W", "TW", "vBat", "vSol", "t"];
var dayNames = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];

var rainOptions = {
	title: '',
	//	curveType: 'function',
	legend: { position: 'bottom' },
	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },


};

var infoOptions = {
	title: '',
	curveType: 'function',
	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
		1: { logScale: false }
	},
	series: {
		0: { targetAxisIndex: 0 },// vBat
		1: { targetAxisIndex: 0 },// vSol
		2: { targetAxisIndex: 1 }// temperature
	},
};

var rawWeightOptions = {
	title: '',
	curveType: 'function',
	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
		1: { logScale: false }
	},
	series: {
		0: { targetAxisIndex: 0 },// raw weight
		1: { targetAxisIndex: 1 }// temperature
	},
};




function clear() {
	infoData.removeRows(0, infoData.getNumberOfRows());
	chart.draw(infoData, options);
	tick = 0;
}


function initPage() {

	table = document.getElementById("dayTable"); //document.createElement("table");
	//	var nrRows = 1;
	var nrColls = 7;

	for (var r = 0; r < 2; r++) {
		var row = table.insertRow(r);
		for (var x = 0; x < nrColls; x++) {
			var cell = row.insertCell(x);
			cell.width = 100; //tableCollWidths[x];
			var item = document.createElement("text")

			var t = document.createTextNode("");
			//	t.id = "d"+x;
			item.appendChild(t);
			cell.appendChild(item);


			if (r == 0)
				cell.innerHTML = dayNames[x];
			else {
				cell.innerHTML = x;
				cell.id = "d" + x;
			}
		}
	}
	document.body.appendChild(table);

}

function test() {
	var td = table.rows[0].cells[1];
	td.width = '150pxx';
	td.style.backgroundColor = 'blue';

}




//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5


function plotInfo(channel, value) {
	if (chartRdy) {
		if (channel == 1) {
			infoData.addRow();
			if (infoData.getNumberOfRows() > MAXPOINTS == true)
				infoData.removeRows(0, infoData.getNumberOfRows() - MAXPOINTS);
		}
		value = parseFloat(value); // from string to float
		infoData.setValue(infoData.getNumberOfRows() - 1, channel, value);
	}
}


function plotRain(channel, value) {
	if (chartRdy) {
		if (channel == 1) {
			rainData.addRow();
			if (rainData.getNumberOfRows() > MAXPOINTS == true)
				rainData.removeRows(0, rainData.getNumberOfRows() - MAXPOINTS);
		}
		value = parseFloat(value); // from string to float
		rainData.setValue(rainData.getNumberOfRows() - 1, channel, value);
	}
}

function plotRawWeight ( channel, value){
	if (chartRdy) {
		if (channel == 1) {
			rawWeightData.addRow();
			if (rawWeightData.getNumberOfRows() > MAXPOINTS == true)
				rawWeightData.removeRows(0, rawWeightData.getNumberOfRows() - MAXPOINTS);
		}
		var val = parseFloat(value); 
		//value = parseFloat(value); // from string to float
		rawWeightData.setValue(rawWeightData.getNumberOfRows() - 1, channel, val);
	}
}


function initChart() {

	rainChart = new google.visualization.ColumnChart(document.getElementById('rain_chart'));
	infoData = new google.visualization.DataTable();
	infoData.addColumn('string', 'Time');
	infoData.addColumn('number', 'vBat');
	infoData.addColumn('number', 'vSol');
	infoData.addColumn('number', 't');
	
	initPage();

	infoChart = new google.visualization.LineChart(document.getElementById('info_chart'));

	rainData = new google.visualization.DataTable();
	rainData.addColumn('string', 'Time');
	rainData.addColumn('number', 'mm');
	//	rainData.addColumn('number', 'w');// temporary

	rawWeightChart = new google.visualization.LineChart(document.getElementById('rawWeight chart'));
	rawWeightData = new google.visualization.DataTable();// temporary to investigate temperature coeff. loadcell
	rawWeightData.addColumn('string', 'Time');
	rawWeightData.addColumn('number', 'Raw');
	rawWeightData.addColumn('number', 't');

	chartRdy = true;
	dontDraw = false;
	if (SIMULATE)
		simplot();
	else
		getValues();
	//startTimer();
}

function startTimer() {
	setInterval(function() { timer() }, 1000);
}
var firstRequest = true;
var plotTimer = 6; // every 60 seconds plot averaged value
var rows = 0;

function updateLastDayTimeLabel(data) {

	var ms = Date.now();

	var date = new Date(ms);
	var labelText = date.getHours() + ':' + date.getMinutes();
	data.setValue(data.getNumberOfRows() - 1, 0, labelText);

}


function updateAllDayTimeLabels(data) {
	var rows = data.getNumberOfRows();
	var minutesAgo = rows * MINUTESPERTICK;
	var ms = Date.now();
	ms -= (minutesAgo * 60 * 1000);
	for (var n = 0; n < rows; n++) {
		var date = new Date(ms);
		var labelText = dayNames[date.getDay()] + ';' + date.getHours() + ':' + date.getMinutes();
		data.setValue(n, 0, labelText);
		ms += 60 * 1000 * MINUTESPERTICK;

	}
}
 
var lastW = -1;
let dayTotal = [0, 0, 0, 0, 0, 0, 0,];

function calcRain(nrpoints, nr, w, tw) {
	var sum = parseFloat(tw);
	var mm = 0;
	if (parseFloat(w) > 0)
		sum += parseFloat(w);// total weight 

	if (lastW >= 0) { // skip first value
		mm = (sum - lastW) / GRAMSPERMM;
		if ( mm > 0.2)
	    	plotRain(1, mm); // plot rain in last 15 minutes
	    else
	        plotRain(1, 0); // skip noise
	    if ( sum >= 0)
            lastW = sum;
        else
            sum = 0;
		//	plotRain(2, w); // test only
	}
	else
		lastW = sum;
	return mm

}


function simplot() {
	var w = 0;
	var str = ",2,3,4,5,\n";
	var str2 = "";
	for (var n = 0; n < 3 * 24 * 4; n++) {
		simValue1 += 0.01;
		simValue2 = Math.sin(simValue1);
		if ((n & 16) > 12)
			w += 20;

//                                         delta  W            W                        RAW                    vBAT                       VSOL                       temperature                                                                     
		str2 = str2 + simMssgCnts++ + "," + simValue2 + "," + w + "," + ( 100 *(simValue2 + 3)) + "," + (simValue2 + 20) +"," + (simValue2* 5)+ "," +  + (simValue2*4)+ "," +"\n";
	}
	plotArray(str2);
}

function plotArray(str) {
	var arr;
	var arr2 = str.split("\n");
	var nrPoints = arr2.length - 1;

	var mm = 0;

	let now = new Date();
	var today = now.getDay();
	var hours = now.getHours();
	var minutes = now.getMinutes();
	var quartersToday = (hours * 4 + minutes / 15);
	var daysInLog = ((nrPoints - quartersToday) / (24 * 4)); // complete days
	if ( daysInLog < 0 )
	    daysInLog = 0;
     daysInLog -= daysInLog%1;
	var dayIdx = today - daysInLog -1 ; // where to start 
	if (dayIdx < 0)
		dayIdx += LOGDAYS;
	var quartersFirstDay = nrPoints - quartersToday - (daysInLog * 24 * 4);// first day probably incomplete
	if (quartersFirstDay < 0)
		quartersFirstDay = nrPoints;

	var quartersToday = 24 * 4;

	for (var p = 0; p < nrPoints; p++) {
		arr = arr2[p].split(",");
		if (arr.length >= 6) {
			mm += calcRain(nrPoints, p, arr[1], arr[2]);
			if (quartersFirstDay > 0) {
				quartersFirstDay--;
				dayTotal[dayIdx] = mm;
				if (quartersFirstDay <= 0) {
		            dayIdx++;
					mm = 0;
				}
			}
			else {
				if (quartersToday > 0)
					quartersToday--;
				if ((quartersToday <= 0) || (p == (nrPoints-1))) {
					dayTotal[dayIdx++] = mm;
					mm = 0;
					quartersToday = 24 * 4;

				}
			}
			if (dayIdx >= LOGDAYS)
				dayIdx = 0;
				plotInfo( 1,arr[4] ); // weight
				plotInfo( 2,arr[5] ); // total weight
				plotInfo( 3,arr[6] ); // temperature
								
				plotRawWeight(1,arr[3]);
				plotRawWeight(2,arr[6]);  // temperature
				
		//	for (var m = 3; m < 6; m++)
		//		plotInfo(m - 2, arr[m]);
		}
		if (nrPoints == 1) { // then single point added 
			updateLastDayTimeLabel(rainData);
			updateLastDayTimeLabel(infoData);
			updateLastDayTimeLabel(rawWeightData);
		}
		else {
			updateAllDayTimeLabels(rainData);
			updateAllDayTimeLabels(infoData);
			updateAllDayTimeLabels(rawWeightData);
		}
	}
	infoChart.draw(infoData, infoOptions);
	rainChart.draw(rainData, rainOptions);
	rawWeightChart.draw(rawWeightData, rawWeightOptions);

	for (var d = 0; d < LOGDAYS; d++) {
		var cellx = document.getElementById("d" + d);
// 		cellx.innerHTML = (dayTotal[d]).toLocaleString(
// 			undefined, // leave undefined to use the visitor's browser 
// 			// locale or a string like 'en-US' to override it.
// 			{ maximumFractionDigits: 1 });
        if 	(dayTotal[d] < 0 )
            dayTotal[d] = 0;
		cellx.innerHTML = dayTotal[d].toFixed(1);

	}

}


function getValues() {
	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "/cgi-bin/getLogMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			plotArray(str);

		}
	}
}




