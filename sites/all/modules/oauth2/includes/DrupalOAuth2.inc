<?php

/**
 * @file
 * Drupal OAuth2 Implementation
 */

class DrupalOAuth2 extends OAuth2 {

  /**
   * Overrides OAuth2::__construct().
   */
  public function __construct($config = array()) {
    global $user;

    parent::__construct($config);

    $oauth2_mode = variable_get('oauth2_provider_mode');
    //only for standalone mode and no need to load if already logged in
    if ($oauth2_mode == 'DrupalOAuth2' && !user_is_logged_in()) {

      // devel_execute_block_form also submit execute code in $_REQUEST['code'].
      if (!isset($_REQUEST['form_id']) || $_REQUEST['form_id'] !== 'devel_execute_block_form') {
        $result = NULL;

        // Hack to corresponding user if oauth_token provided.
        if (isset($_REQUEST['oauth_token'])) {
          $result = $this->getAccessToken($_REQUEST['oauth_token']);
        } elseif (isset($_REQUEST['access_token'])) {
          $result = $this->getAccessToken($_REQUEST['access_token']);
        }
        // Hack to corresponding user if code provided.
        elseif (isset($_REQUEST['code'])) {
          $result = $this->getAuthCode($_REQUEST['code']);
        }
        // Hack to corresponding user if refresh_token provided.
        elseif (isset($_REQUEST['refresh_token'])) {
          $result = $this->getRefreshToken($_REQUEST['refresh_token']);
        }

        if ($result) {
          $this->loadUserCredentials($result['uid'], $result['name'], $result['sid']);
        }
      }

    }
  }

  /**
   * Load the user based on uid and name, assign with corresponding session;
   * or create dummy user on-the-fly.
   *
   * @param $uid
   *   The user ID.
   * @param $name
   *   The user name.
   * @param $sid
   *   The target session ID to be set with.
   *
   * @return
   *   The user object after successful login.
   */
  protected function loadUserCredentials($uid, $name, $sid) {
    global $user;

    //user_external_login_register($name,'oauth2');
    if (isset($uid) && isset($name) && !($account = user_load($uid))) {
      // We hack and create a dummy user placeholder if it is exist in
      // token tables but not yet a real user account, typically
      // happened when server running in proxy mode. Can't use
      // drupal_write_record() as uid is serial.
      //db_query("INSERT INTO {users} (uid, name, created, access, login, status) VALUES (:uid, :name, :created, :access, :login, 1)", array(':uid' => $uid, ':name' => $name, ':created' => time(), ':access' => time(), ':login' => time()));
      //$account = user_load($uid);
      return FALSE;
    }
    $user = $account;

    //login the user
    //copy from user_login_finalize without calling user_module_invoke('login', $edit, $user);
    watchdog('user', 'Session opened for %name by oauth2.', array('%name' => $user->name));
    // Update the user table timestamp noting user has logged in.
    // This is also used to invalidate one-time login links.
    $user->login = REQUEST_TIME;
    db_update('users')
    ->fields(array('login' => $user->login))
    ->condition('uid', $user->uid)
    ->execute();

    // Regenerate the session ID to prevent against session fixation attacks.
    // This is called before hook_user in case one of those functions fails
    // or incorrectly does a redirect which would leave the old session in place.
    drupal_session_regenerate();

    // Some client/servers, like XMLRPC, do not handle cookies, so
    // imitate it to make sess_read() function try to look for user,
    // instead of just loading anonymous user :).
    //$session_name = session_name();
    //if (!isset($_COOKIE[$session_name])) {
    //  $_COOKIE[$session_name] = $sid;
    //}

    //session_id($sid);

    return $user;
  }

  /**
   * Verify authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be check with.
   * @param $required_scope
   *   The required scope within current request.
   * @param $uid
   *   The user ID.
   *
   * @return
   *   A list of all extended scopes besides authorized scopes of end-user.
   */
  public function verifyAuthorizedScopes($client_id, $required_scope, $uid) {
    $authorized_scope = $this->getAuthorizedScopes($client_id, $uid);
    $hidden_scope = $this->getHiddenScopes();

    if (!is_array($required_scope))
      $required_scope = explode(" ", $required_scope);
    $required_scope = array_unique(array_merge($required_scope, $hidden_scope));

    $scopes = $this->checkExtendedPermissions($required_scope, $authorized_scope);

    $extended_permissions = array();
    foreach ($scopes as $scope) {
      $result = oauth2_scope_load($scope);
      $extended_permissions[] = $result;
    }

    return array_filter($extended_permissions);
  }

  /**
   * Get all hidden scopes that should be automatically included within blank
   * scope request.
   *
   * @return
   *   A list for all hidden scopes.
   */
  public function getHiddenScopes() {
    $result = db_query("SELECT os.scope_id FROM  {oauth2_scope} AS os  WHERE os.hidden = 1 ");
    $scopes = array();
    foreach ($result as $record) {
      $scopes[] = $record['scope_id'];
    }
    return $scopes;
  }

  /**
   * Get all authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be check with.
   * @param $uid
   *   The user ID.
   *
   * @return
   *   A list for all authorized scopes for this end-user.
   */
  public function getAuthorizedScopes($client_id, $uid) {
    $result = db_query("SELECT scope_id FROM {oauth2_authorize} WHERE client_id = :s AND uid = :d", array(':s' => $client_id, ':d' => $uid));
    $scopes = array();
    foreach ($result as $record) {
      $scopes[] = $record['scope_id'];
    }
    return $scopes;
  }

  /**
   * Set authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be authorize.
   * @param $scopes
   *   The scopes as a list of space-delimited strings.
   * @param $uid
   *   The user ID.
   */
  public function setAuthorizedScopes($client_id, $scopes, $uid) {
    $scope_ids = explode(" ", $scopes);
    foreach ($scope_ids as $scope_id) {
      db_query("INSERT INTO {oauth2_authorize} (client_id, uid, scope_id) VALUES (:client_id, :uid, :scope_id)", array(':client_id' => $client_id, ':uid' => $uid, 'scope_id' => $scope_id));
    }
  }

  /**
   * Unset authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be unauthorize.
   * @param $scopes
   *   The scopes as a list of space-delimited strings.
   * @param $uid
   *   The user id.
   */
  public function unsetAuthorizedScopes($client_id, $scopes, $uid) {
    $scope_ids = explode(" ", $scopes);
    foreach ($scope_ids as $scope_id) {
      db_query("DELETE FROM {oauth2_authorize} WHERE client_id = :client_id AND uid = :uid AND scope_id = :scope_id", array(':client_id' => $client_id, ':uid' => $uid, ':scope_id' => $scope_id));
    }
  }

  /**
   * Check if any extended permissions besides authorized scopes.
   *
   * @param $required_scope
   *   The target required scope within current request.
   * @param $authorized_scope
   *   Scopes that already authorized by end-user.
   *
   * @return
   *   A list with all extended permissions.
   */
  private function checkExtendedPermissions($required_scope, $authorized_scope) {
    // The required scope should match or be a subset of the available scope
    if (!is_array($required_scope))
      $required_scope = explode(" ", $required_scope);

    if (!is_array($authorized_scope))
      $authorized_scope = explode(" ", $authorized_scope);

    return array_filter(array_diff($required_scope, $authorized_scope));
  }

  /**
   * Expires all OAuth2.0 related tokens based on sid.
   *
   * @param $sid
   *   Session ID to be expires with.
   *
   * @return
   *   TRUE if successful, and FALSE if it isn't.
   */
  public function expireSession($sid = NULL) {

    //global $user;
    //for backward compatability only
    if (isset($_REQUEST['oauth_token']) && !isset($_REQUEST['access_token'])) {
      $_REQUEST['access_token'] = $_REQUEST['oauth_token'];
    }

    // Purge tokens based on oauth_token if provided.
    if (isset($_REQUEST['access_token']) && !empty($_REQUEST['access_token'])) {
      $result = $this->getAccessToken($_REQUEST['access_token']);
      if ($result) {
        $sid = $result['sid'];
      }
    }

    // Purge tokens if session found.
    if ($sid) {
      if (!isset($_GET['redirect_uri'])) {
        // We hack $_REQUEST['redirect_uri'] so handle it custom logout.
        $result = db_query("SELECT oc.redirect_uri
        FROM {oauth2_access_tokens} AS oat
        INNER JOIN {oauth2_client} AS oc ON oat.client_id = oc.client_id
        WHERE oat.sid = :sid ", array(':sid' =>$sid));
        $record = $result->fetchAssoc();
        $_GET['redirect_uri'] = $record['redirect_uri'];

      }

      $result = db_query("SELECT uid FROM {oauth2_access_tokens} WHERE sid = :sid", array(':sid' => $sid));
      $uid = $result->fetchField();

      // Expirse all tokens base on this session_id.
      db_query("DELETE FROM {oauth2_access_tokens} WHERE sid = :sid", array(':sid' => $sid));
      db_query("DELETE FROM {oauth2_auth_codes} WHERE sid = :sid", array(':sid' => $sid));
      db_query("DELETE FROM {oauth2_refresh_tokens} WHERE sid = :sid", array(':sid' => $sid));

      if ($uid != 0) {
        db_query("DELETE FROM {sessions} WHERE uid = :uid", array(':uid' => $uid));
      }

      // Also manually destroy user session.
      /*if (session_id($sid) != $sid) {
      session_start();
      session_id($sid);
      }*/
      return TRUE;
      //return session_destroy();
    }

    return FALSE;
  }


  /**
   * Overrides OAuth2:grantAccessToken()
   */
  public function grantAccessToken() {
    /*global $user;

    if (isset($_POST['grant_type'], $_POST['code']) && $_POST['grant_type'] == OAUTH2_GRANT_TYPE_AUTH_CODE) {
    $stored =  $this->getAuthCode($_POST['code']);
    $uid = $stored['uid'];

    //if it is not non-zero value
    if (!empty($uid) && ($user = user_load($uid))) {

    //login the user
    //watchdog('user', 'Session opened for %name.', array('%name' => $user->name));
    $user->login = REQUEST_TIME;
    db_update('users')
    ->fields(array('login' => $user->login))
    ->condition('uid', $user->uid)
    ->execute();
    }
    }*/

    parent::grantAccessToken();
  }

  /**
   * Implements OAuth2::checkClientCredentials().
   */
  public function checkClientCredentials($client_id, $client_secret = NULL) {
    $result = oauth2_client_load($client_id);
    $record_client_secret = $result->client_secret;
    return ($client_secret !== NULL && $record_client_secret == $client_secret) ? TRUE : FALSE;
  }

  /**
   * Implements OAuth2::getRedirectUri().
   */
  public function getRedirectUri($client_id) {
    $result = oauth2_client_load($client_id);
    $record_redirect_uri = $result->redirect_uri;
    return $record_redirect_uri !== FALSE ? $record_redirect_uri : NULL;
  }

  /**
   * Implements OAuth2::getAccessToken().
   */
  public function getAccessToken($oauth_token) {
    $result = db_query("SELECT oat.client_id, oat.sid, oat.expires, oat.scope, u.uid, u.name, u.mail
    FROM {oauth2_access_tokens} AS oat
    INNER JOIN {users} AS u ON oat.uid = u.uid
    WHERE oat.oauth_token = :s", array(':s' => $oauth_token));
    $temp_result_array = $result->fetchAssoc();
    if ($temp_result_array['uid']) {
      $user = user_load($temp_result_array['uid']);
      unset($user->pass);
      //unset($user->mail);
      //unset($user->uid);
      //unset($user->name);
      //error_log(print_r($user,true));
      $user_array = (array) $user;
      $result_array = array_merge($temp_result_array,$user_array);
      //error_log(print_r($user_array,true));
      //error_log('got uid: '.$user->uid);
    } else {
      $result_array = $temp_result_array;
    }
    
    //error_log(print_r($result_array,true));
    
    return $result_array;
  }

  /**
   * Implements OAuth2::setAccessToken().
   */
  public function setAccessToken($oauth_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return db_query("INSERT INTO {oauth2_access_tokens} (oauth_token, client_id, expires, scope, uid, sid) VALUES (:oauth_token, :client_id, :expires, :scope, :uid, :sid)", array(':oauth_token' => $oauth_token, ':client_id' => $client_id, ':expires' => $expires, ':scope' => $scope, ':uid' => $uid, ':sid' => $sid));
  }

  /**
   * Overrides OAuth2::getSupportedGrantTypes().
   */
  protected function getSupportedGrantTypes() {
    return array(
        OAUTH2_GRANT_TYPE_AUTH_CODE,
        OAUTH2_GRANT_TYPE_USER_CREDENTIALS,
        OAUTH2_GRANT_TYPE_REFRESH_TOKEN
    );
  }

  /**
   * Overrides OAuth2::getSupportedScopes().
   */
  public function getSupportedScopes() {
    $result = db_query("SELECT os.scope_id FROM {oauth2_scope} AS os ");
    $scope_ids = array();
    foreach ($result as $record) {
      $scope_ids[] = $record['scope_id'];
    }
    return $scope_ids;
  }

  /**
   * Overrides OAuth2::getAuthCode().
   */
  public function getAuthCode($code) {
    $result = db_query("SELECT oac.client_id, oac.sid, oac.redirect_uri, oac.expires, oac.scope, u.uid, u.name
    FROM {oauth2_auth_codes} AS oac
    INNER JOIN {users} AS u ON oac.uid = u.uid
    WHERE oac.code = :code", array(':code' => $code));
    return $result->fetchAssoc();
  }

  /**
   * Overrides OAuth2::setAuthCode().
   */
  public function setAuthCode($code, $client_id, $redirect_uri, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return db_query("INSERT INTO {oauth2_auth_codes} (code, client_id, redirect_uri, expires, scope, uid, sid) VALUES (:code, :client_id, :redirect_uri, :expires, :scope, :uid, :sid)", array(':code' => $code, ':client_id' => $client_id, ':redirect_uri' => $redirect_uri, ':expires' => $expires, ':scope' => $scope, ':uid' => $uid, ':sid' => $sid));
  }

  /**
   * Overrides OAuth2::checkUserCredentials().
   */
  public function checkUserCredentials($client_id, $username, $password) {
    // Clone from user_service_login().
    $uid = user_authenticate($username, $password);

    $result = NULL;
    if ($uid) {
      $account = user_load($uid);
      // Regenerate the session ID to prevent against session fixation attacks.
      drupal_session_regenerate();

      $array = array();
      user_module_invoke('login', $array, $account);

      $result = array(
          'uid' => $account->uid,
          'name' => $account->name,
          'sid' => session_id(),
          'scope' => trim(implode(' ', $this->getAuthorizedScopes($client_id, $account->uid))),
      );
    }

    return $result ? $result : FALSE;
  }

  /**
   * Overrides OAuth2::getRefreshToken().
   */
  public function getRefreshToken($refresh_token) {
    $result = db_query("SELECT ort.client_id, ort.sid, ort.expires, ort.scope, u.uid, u.name
    FROM {oauth2_refresh_tokens} AS ort
    INNER JOIN {users} AS u ON ort.uid = u.uid
    WHERE ort.refresh_token = :token", array(':token' => $refresh_token));
    return $result->fetchAssoc();
  }

  /**
   * Overrides OAuth2::setRefreshToken().
   */
  public function setRefreshToken($refresh_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return db_query("INSERT INTO {oauth2_refresh_tokens} (refresh_token, client_id, expires, scope, uid, sid) VALUES (:refresh_token, :client_id, :expires, :scope, :uid, :sid)", array(':refresh_token' => $refresh_token, ':client_id' => $client_id, ':expires' => $expires, ':scope' => $scope, ':uid' => $uid, ':sid' => $sid));
  }

  /**
   * Overrides OAuth2::unsetRefreshToken().
   */
  public function unsetRefreshToken($refresh_token) {
    return db_query("DELETE FROM {oauth2_refresh_tokens} WHERE refresh_token = :s", array(':s' => $refresh_token));
  }
}
