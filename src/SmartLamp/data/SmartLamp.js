var myOldActuals = 0;
var themes = null;

function onload() {
  switchLanguage();

  var language = window.navigator.userLanguage || window.navigator.language;
  // Browser-Sprache auslesen
  if(language.indexOf('de') !== -1) {
    lang = 'de';
  } else {
    lang = 'en';
  }
  readConfig(lang);

  getTimeCall(makeTheCall, 1000);
  makeTheCall();
  debugCall();
}

function packDays(weekDays) {
  let packedDays = 0;
  for (let i = 0; i < 7; i++) {
    packedDays |= (weekDays[i] << i);
  }
  return packedDays;
}

function unpackDays(packedDays, weekDays) {
  for (let i = 0; i < 7; i++) {
    weekDays[i] = ((packedDays >> i) & 1) > 0;
  }
}

function valueChanged(iD) {
  switch (iD) {
    case 'NEXT':
      var newValue = myOldActuals.theme + 1;
      if (newValue == 7) {
        newValue = 9;
      }
      if (newValue > 10) {
        newValue = 0;
      }
      if (!isNaN(newValue))
        setTimeValues({
          "theme": newValue
        });
      break;
    case 'PREV':
      var newValue = myOldActuals.theme - 1;
      if (newValue == 8) {
        newValue = 6;
      }
      if (newValue < 0) {
        newValue = 10;
      }
      if (!isNaN(newValue))
        setTimeValues({
          "theme": newValue
        });
      break;
    case 'colorpicker':
      var colorValue = parseInt($('#colorpicker')[0].value.replace(/^#/, ''), 16);
      setTimeValues({
        "color": colorValue
      });
      break;
    case 'dawnMonday':
    case 'dawnTuesday':
    case 'dawnWednesday':
    case 'dawnThursday':
    case 'dawnFriday':
    case 'dawnSaturday':
    case 'dawnSunday':
      var dawn_days = new Array(7).fill(false);
      dawn_days[0] = document.getElementById('dawnMonday').checked;
      dawn_days[1] = document.getElementById('dawnTuesday').checked;
      dawn_days[2] = document.getElementById('dawnWednesday').checked;
      dawn_days[3] = document.getElementById('dawnThursday').checked;
      dawn_days[4] = document.getElementById('dawnFriday').checked;
      dawn_days[5] = document.getElementById('dawnSaturday').checked;
      dawn_days[6] = document.getElementById('dawnSunday').checked;
      var doDawn = packDays(dawn_days);

      setTimeValues({
        "do_dawn": doDawn
      });
      break;
    case 'duskMonday':
    case 'duskTuesday':
    case 'duskWednesday':
    case 'duskThursday':
    case 'duskFriday':
    case 'duskSaturday':
    case 'duskSunday':
      var dusk_days = new Array(7).fill(false);
      dusk_days[0] = document.getElementById('duskMonday').checked;
      dusk_days[1] = document.getElementById('duskTuesday').checked;
      dusk_days[2] = document.getElementById('duskWednesday').checked;
      dusk_days[3] = document.getElementById('duskThursday').checked;
      dusk_days[4] = document.getElementById('duskFriday').checked;
      dusk_days[5] = document.getElementById('duskSaturday').checked;
      dusk_days[6] = document.getElementById('duskSunday').checked;
      var doDusk = packDays(dusk_days);

      setTimeValues({
        "do_dusk": doDusk
      });
      break;
    default:
      var theme = parseInt(iD);
      if (!isNaN(theme))
        setTimeValues({
          "theme": theme
        });
      break;
  }
}

function onClickOk() {
  var dawnSplit = $('#DAWN_SET')[0].value.split(":");
  var hourDawnValue = parseInt(Math.abs(dawnSplit[0]));
  var minuteDawnValue = parseInt(Math.abs(dawnSplit[1]));

  var duskSplit = $('#DUSK_SET')[0].value.split(":");
  var hourDuskValue = parseInt(Math.abs(duskSplit[0]));
  var minuteDuskValue = parseInt(Math.abs(duskSplit[1]));

  if (!((isNaN(hourDawnValue)) | (hourDawnValue > 23.0) | (hourDawnValue < 0)) && !((isNaN(minuteDawnValue)) | (minuteDawnValue > 59) | (minuteDawnValue < 0)) &&
    !((isNaN(hourDuskValue)) | (hourDuskValue > 23.0) | (hourDuskValue < 0)) && !((isNaN(minuteDuskValue)) | (minuteDuskValue > 59) | (minuteDuskValue < 0)))
    setTimeValues({
      "hour_dawn": hourDawnValue,
      "minute_dawn": minuteDawnValue,
      "hour_dusk": hourDuskValue,
      "minute_dusk": minuteDuskValue
    });
  else
    getTimeCall();
}

function setTimeValues(cdo) {
  $.ajaxSetup({
    async: true,
    cache: false
  });
  var def2 = $.ajax({
    method: "POST",
    url: 'rest',
    data: JSON.stringify(cdo),
    contentType: "application/json;",
    dataType: "json",
    headers: {
      'accept': 'application/json',
      'Cache-Control': 'no-store'
    },
  });
  def2.done(function (data) {
    setTimeout(getTimeCall, 200);
  });
}

function textSelector(element) {
  /*bei Auswahl des Eingabefeldes wird der gesamte Inhalt markiert*/
  $(element).select();
}

function readConfig(lang) {
  $.ajaxSetup({
    async: true,
    cache: false
  });
  var def = $.ajax({
    method: "POST",
    url: 'readconfig',
    data: lang,
    contentType: "text/plain;",
    headers: {
      'accept': 'application/json',
      'Cache-Control': 'no-store'
    },
  });
  def.done(function (data) {
    if (data != "") {
      //console.log(data);
      themes = data;

      for(i = 0; i < themes.length; i++)
      {
        element = document.getElementById(i);
        if (element != null)
          element.innerHTML = themes[i];
      }
    };

    setTimeout(makeTheCall, 200);
  })
}

function makeTheCall() {
  $.ajaxSetup({
    async: true,
    cache: false
  });
  var def = $.ajax({
    method: "GET",
    url: 'read',
    headers: {
      'accept': 'application/json',
      'Cache-Control': 'no-store'
    },
  });
  def.done(function (data) {
    if (data != "") {
      //console.log(data);
      var myActuals = data;

      if (myActuals.theme != myOldActuals.theme) {
        if(themes == null)
          document.getElementById('_ledTheme').innerHTML = "LED Thema " + parseInt(myActuals.theme);
        else
          document.getElementById('_ledTheme').innerHTML = "LED Thema " + themes[parseInt(myActuals.theme)];
        _ledTheme = myActuals.theme;
      }
      myOldActuals = myActuals;
    };

    setTimeout(makeTheCall, 200);
  })
}

function debugCall() {
  $.ajaxSetup({
    async: true,
    cache: false
  });
  var def = $.ajax({
    method: "GET",
    url: 'debug',
    headers: {
      'accept': 'text/plain',
      'Cache-Control': 'no-store'
    },
  });
  def.done(function (data) {
    if (data != "") {
      console.log(data);
      document.getElementById('DEBUG').innerHTML = data;
    };

    setTimeout(debugCall, 200);
  })
}

function getTimeCall() {
  $.ajaxSetup({
    async: true,
    cache: false
  });
  var def = $.ajax({
    method: "GET",
    url: 'read',
    headers: {
      'accept': 'application/json',
      'Cache-Control': 'no-store'
    },
  });
  def.done(function (data) {
    if (data === "") {
      return
    };
    var myActuals = data;
    document.getElementById('DAWN_SET').value = parseInt(myActuals.hour_dawn).toString().padStart(2, '0') + ":" + parseInt(myActuals.minute_dawn).toString().padStart(2, '0');
    document.getElementById('DUSK_SET').value = parseInt(myActuals.hour_dusk).toString().padStart(2, '0') + ":" + parseInt(myActuals.minute_dusk).toString().padStart(2, '0');

    var dawn_days = new Array(7).fill(false);
    unpackDays(parseInt(myActuals.do_dawn), dawn_days);
    document.getElementById('dawnMonday').checked = dawn_days[0];
    document.getElementById('dawnTuesday').checked = dawn_days[1];
    document.getElementById('dawnWednesday').checked = dawn_days[2];
    document.getElementById('dawnThursday').checked = dawn_days[3];
    document.getElementById('dawnFriday').checked = dawn_days[4];
    document.getElementById('dawnSaturday').checked = dawn_days[5];
    document.getElementById('dawnSunday').checked = dawn_days[6];

    var dusk_days = new Array(7).fill(false);
    unpackDays(parseInt(myActuals.do_dusk), dusk_days);
    document.getElementById('duskMonday').checked = dusk_days[0];
    document.getElementById('duskTuesday').checked = dusk_days[1];
    document.getElementById('duskWednesday').checked = dusk_days[2];
    document.getElementById('duskThursday').checked = dusk_days[3];
    document.getElementById('duskFriday').checked = dusk_days[4];
    document.getElementById('duskSaturday').checked = dusk_days[5];
    document.getElementById('duskSunday').checked = dusk_days[6];

    document.getElementById('colorpicker').value = "#" + parseInt(myActuals.color).toString(16).padStart(6, '0');
  })
}

function switchLanguage() {
  //Das Javascript so einfach wie Möglich
  var language = window.navigator.userLanguage || window.navigator.language;
  // Browser-Sprache auslesen
  if(language.indexOf('de') !== -1) {
    testsprache = 'de';
  } else {
    testsprache = 'en';
  }
  // Browser-Sprache an den Body anhängen
  $('body').attr( "id",'lang-'+testsprache);
  // Sprache wechseln (hier mit jquery auf bestimmte Links)
  $(".languageswitcher").on('click',function(e){
    e.preventDefault();
  $("body").attr( "id",'lang-'+$(this).attr('lang'));
  });
}