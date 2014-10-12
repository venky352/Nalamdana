<?php

function dprint_r($msg) {
  echo "<code><pre>";
  var_dump($msg);
  echo "</pre></code>";
}

/**
 * @file
 * Drupal OAuth2.0 client library platform independent testing console.
 */

require_once '../../../libraries/oauth2-php/lib/OAuth2.inc';
require_once '../../../libraries/oauth2-php/lib/OAuth2Client.inc';
require_once '../../../libraries/oauth2-php/lib/OAuth2Exception.inc';
require_once 'DrupalOAuth2Client.inc';

// Create our Application instance (replace this with your appId and secret).
$drupal = new DrupalOAuth2Client(array(
    'client_id' => '2a0d4c44690fbae5c94871686fb9408d',
    'client_secret' => '2c407366f8c5d28d67bf836d4c5c369e',
    'base_uri' => 'http://server7.oauth2test.pantarei-design.com/',//http://dev.client7.oauth2.pantarei-design.com/',
    'authorize_uri' => 'oauth2/authorize',
    'access_token_uri' => 'oauth2/access_token',
    'services_uri' => 'testing',
    'expires_in' => 3600,
    'cookie_support' => TRUE,
));

// We may or may not have this data based on a $_GET or $_COOKIE based session.
//
// If we get a session here, it means we found a correctly signed session using
// the Application Secret only Drupal OAuth2.0 and the Application know. We dont know
// if it is still valid until we make an API call using the session. A session
// can become invalid if it has already expired (should not be getting the
// session back in this case) or if the user logged out of Drupal OAuth2.0.
$session = $drupal->getSession();

$me = NULL;
// Session based API call.
if ($session) {
  try {
    $me = $drupal->api('/user/retrieve');
    //$uid = $me['uid'];
    $uid = $me['uid'];
  }
  catch (OAuth2Exception $e) {
    error_log($e);
  }
}

// login or logout url will be needed depending on current user state.
if ($me) {
  $logout_uri = $drupal->getLogoutUri();
}
else {
  $login_uri = $drupal->getLoginUri();
}

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en"
  dir="ltr">
<head>
<title>DrupalOAuth2Client</title>
<style>
body {
  font-family: 'Lucida Grande', Verdana, Arial, sans-serif;
}

h1 a {
  text-decoration: none;
  color: #3b5998;
}

h1 a:hover {
  text-decoration: underline;
}
</style>
</head>
<body>
  <?php if (!empty($_GET)) : ?>
  <script type="text/javascript">
        window.location = "<?php echo $_SERVER['SCRIPT_NAME']; ?>"
      </script>
  <?php endif ?>

  <h1>
    <a href="<?php echo $_SERVER['SCRIPT_NAME']; ?>">DrupalOAuth2Client</a>
  </h1>

  <?php if ($me): ?>
  <a href="<?php echo $logout_uri; ?>"> Logout </a>
  <?php else: ?>
  <div>
    Without using JavaScript &amp; XFBML: <a
      href="<?php echo $login_uri; ?>"> Login </a>
  </div>
  <div>
    <form id="Oauth_Login" method="post">
      <p>
        <label for="id">Client ID:</label> <input type="text"
          name="username" id="username" />
      </p>
      <p>
        <label for="pw">Client Secret (password/key):</label> <input
          type="password" name="password" id="password" />
      </p>
      <input type="submit" value="Submit" />
    </form>
  </div>
  <?php endif ?>

  <h3>Session</h3>
  <?php if ($me): ?>
  <pre>
    <?php print_r($session); ?>
  </pre>

  <h3>You</h3>
  <img src="/p/profile/<?php echo $uid; ?>/picture">
  <?php echo $me['name']; ?>

  <h3>Your User Object</h3>
  <pre>
    <?php print_r($me); ?>
  </pre>
  <?php else: ?>
  <strong><em>You are not Connected.</em> </strong>
  <?php endif ?>

</body>
</html>
