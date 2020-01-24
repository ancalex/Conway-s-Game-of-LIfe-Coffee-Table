#ifndef PAGE_GAMES_H
#define PAGE_GAMES_H

const char PAGE_Games[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s btn--blue">&#9664;</a>&nbsp;&nbsp;<strong>Game Chooser</strong>
<hr>
<form action="" method="get">
<table border="0"  cellspacing="0" cellpadding="3" >
<tr><td align="right">Density (1-99)</td><td><input id="density" name="density" type="text" value="" style="width:150px"></td></tr>
<tr><td align="right"></td><td> </td></tr>
<tr><td align="right">Fading Step (1-254)</td><td><input id="fading_step" name="fading_step" type="text" value="" style="width:150px"></td></tr>
<tr><td align="right"> </td><td></td></tr>
<tr><td align="right">Select Game</td><td>
<select  id="game" name="game">
  <option value="0">Conway's Game of Life</option>
  <option value="1">Amoeba</option>
  <option value="2">Assimilation</option>
  <option value="3">Coral</option>
  <option value="4">HighLife</option>
  <option value="5">Gnarl</option>
  <option value="6">LongLife</option>
  <option value="7">Replicator</option>
</select>
</td></tr>
<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>
<script>
  
window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
        setValues("/admin/gamesvalues");
    });
  });
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}

</script>
)=====";

void send_games_html()
{
	Serial.println("************************************");
	Serial.println(server.args());

	  if (server.args() > 0 )  // Save Settings
	  {
	    String temp = "";
	    for ( uint8_t i = 0; i < server.args(); i++ ) {
	      if (server.argName(i) == "density") config.density = server.arg(i).toInt();
	      if (server.argName(i) == "fading_step") config.fading_step =  server.arg(i).toInt();
	      if (server.argName(i) == "game") config.game =  server.arg(i).toInt();
	    }

	    //Serial.println(config.game);
	    WriteConfig();

	  }
	  server.send_P ( 200, "text/html", PAGE_Games );
	  Serial.println(__FUNCTION__);
}

void send_games_values_html()
{

  String values ="";
  values += "density|" + (String) config.density + "|input\n";
  values += "fading_step|" +  (String) config.fading_step + "|input\n";
  values += "game|" +  (String) config.game + "|input\n";
  //Serial.println(values);
  server.send ( 200, "text/plain", values);
  Serial.println(__FUNCTION__);
  AdminTimeOutCounter=0;
  generation = 0;
}

#endif /* PAGE_GAMES_H */

