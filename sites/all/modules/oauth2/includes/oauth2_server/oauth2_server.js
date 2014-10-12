function getParameterByName(name) {
  var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
  if (!match) {
    return false;
  }
  return decodeURIComponent(match[1].replace(/\+/g, ' '));
}

jQuery(document).ready(
    function($) {
      // if inside iframe
      if (window != parent && getParameterByName('code') != false) {
        parent.location.reload();
        return;
      }

      function getCookie(c_name) {
        var i, x, y, ARRcookies = document.cookie.split(";");
        for (i = 0; i < ARRcookies.length; i++) {
          x = ARRcookies[i].substr(0, ARRcookies[i].indexOf("="));
          y = ARRcookies[i].substr(ARRcookies[i].indexOf("=") + 1);
          x = x.replace(/^\s+|\s+$/g, "");
          if (x == c_name) {
            return unescape(y);
          }
        }
      }

      function setCookie(pLabel, pVal, pMinutes) {
        var tExpDate = new Date();
        tExpDate.setTime(tExpDate.getTime() + (pMinutes * 60 * 1000));
        document.cookie = pLabel + "=" + escape(pVal)
            + ((pMinutes == null) ? "" : ";expires=" + tExpDate.toGMTString());
      }

      var ifrm = document.createElement("IFRAME");
      ifrm.style.width = 640 + "px";
      ifrm.style.height = 480 + "px";
      ifrm.style.display = 'none';
      ifrm.id = 'oauth2_iframe';

      document.body.appendChild(ifrm);
      // console.debug(Drupal.settings.oauth2.url);
      if (Drupal.settings.oauth2 && !Drupal.settings.oauth2.logged_in
          && Drupal.settings.oauth2.auto_login_enabled) {
          //&& getCookie('oauth2_check_login') == null) {
        //console.debug('set cookie');
        //setCookie('oauth2_check_login', 'checked', 10);
        $("#oauth2_iframe").attr(
            'src',
            Drupal.settings.oauth2.url + Drupal.settings.oauth2.authorize_uri
                + '?response_type=code&client_id='
                + Drupal.settings.oauth2.client_id + '&redirect_uri='
                + Drupal.settings.oauth2.redirect_uri);
      } else {
        //console.debug('not loaded');
      }

      $('#oauth2_login_iframe').click(
          function() {

            var iframe = $("#oauth2_iframe").detach();
            $(this).parent().append(iframe);
            if (!$(iframe).attr('src')) {
              $(iframe).attr(
                  'src',
                  Drupal.settings.oauth2.url
                      + Drupal.settings.oauth2.authorize_uri
                      + '?response_type=code&client_id='
                      + Drupal.settings.oauth2.client_id + '&redirect_uri='
                      + Drupal.settings.oauth2.redirect_uri);
            }
            $(iframe).show();
          });
    });