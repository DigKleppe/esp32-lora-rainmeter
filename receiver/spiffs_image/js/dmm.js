


function startDMM() {
	setTimeout(function() { start() }, 3000);
}

function start() {
	if (SIMULATE)
		setInterval(simp, 1000);
	else
		setInterval(readMeasValues, 1000);
}

function simp() {
	var arr = simplot();
	document.ActualValues.AveragedValue.value = arr;
	document.ActualValues.MomentaryValue.value = arr + 10;
}


function readMeasValues() {
	var arr = getMeasValues();
	document.ActualValues.AveragedValue.value = arr[1];
	document.ActualValues.MomentaryValue.value = arr[2];
}
