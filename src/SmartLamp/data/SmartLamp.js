var myOldActuals = 0;

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
    case 'dawnCheck':
      var doDawn = $('#dawnCheck')[0].checked;
      setTimeValues({
        "do_dawn": doDawn
      });
    case 'duskCheck':
      var doDusk = $('#duskCheck')[0].checked;
      setTimeValues({
        "do_dusk": doDusk
      });
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
      var config = data;

      for(i = 0; i < config.length; i++)
      {
        element = document.getElementById(i);
        if (element != null)
          element.innerHTML = config[i];
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
        document.getElementById('_ledTheme').innerHTML = "LED Thema " + parseInt(myActuals.theme);
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

    document.getElementById('dawnCheck').checked = Boolean(myActuals.do_dawn);
    document.getElementById('duskCheck').checked = Boolean(myActuals.do_dusk);
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