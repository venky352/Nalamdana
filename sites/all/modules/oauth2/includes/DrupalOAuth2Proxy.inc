<?php

/**
 * @file
 * Drupal OAuth2 Proxy mode Implementation.
 */

/**
 * Drupal OAuth2 Proxy mode Implementation.
 */
class DrupalOAuth2Proxy extends DrupalOAuth2 {

  /**
   * Default options for DrupalOAuth2Client.
   */
  protected static $DrupalOAuth2Client_OPTS = array(
      'cookie_support' => FALSE,
  );

  /**
   * Overrides OAuth2::__construct().
   */
  public function __construct($config = array()) {
    // Remember to clean cache that out-dated.
    cache_clear_all(NULL, 'cache_oauth2', TRUE);

    // $client is used within getAccessToken() so we MUST first
    // initialize it.
    $opts = self::$DrupalOAuth2Client_OPTS;
    $client = oauth2_get_client(
    variable_get('oauth2_proxy_server_identifier', NULL),
    $opts
    );
    $client->getSession();
    $this->setVariable('_client', $client);


    // devel_execute_block_form also submit execute code in $_REQUEST['code'].
    if (!isset($_REQUEST['form_id']) || $_REQUEST['form_id'] !== 'devel_execute_block_form') {
      $result = NULL;

      // Hack to corresponding user if oauth_token provided. $_REQUEST may not contain $_COOKIE for most server setting
      if (isset($_REQUEST['oauth_token'])) {
        $result = $this->getAccessToken($_REQUEST['oauth_token']);
      }
      // oauth_token may be inside cookie
      elseif (isset($_COOKIE['oauth_token'])) {
        $result = $this->getAccessToken($_COOKIE['oauth_token']);
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
        /*$access_token = $client->getAccessToken();

        //save the access token into cookie
        $params = session_get_cookie_params();
        $expire = $params['lifetime'] ? REQUEST_TIME + $params['lifetime'] : 0;
        setcookie('oauth_token', $access_token, $expire, $params['path'], $params['domain'], FALSE, $params['httponly'] );*/
        $this->loadUserCredentials($result['uid'], $result['name'], $result['sid']);
      }
    }
    parent::__construct($config);
  }

  /**
   * Wrapper of DrupalOAuth2Client::api().
   *
   * Will handle with additional local cache in order to boost the performance
   * and reduce remote server loading.
   *
   * @param $path
   *   The target path, relative to base_path/service_uri or an absolute URI.
   * @param $method
   *   (optional) The HTTP method (default 'GET').
   * @param $params
   *   (optional The GET/POST parameters.
   *
   * @return
   *   The JSON decoded response object.
   *
   * @throws OAuth2Exception
   */
  protected function proxyApi($path, $method = 'POST', $params = array()) {
    // Similar as original api() but we default with POST.
    if (is_array($method) && empty($params)) {
      $params = $method;
      $method = 'POST';
    }

    $cache_params = $params;
    $cache_params['path'] = $path;
    $cache_params['method'] = $method;
    $cache_key = "oauth2:". md5(serialize($cache_params));

    $cache_result = new stdClass();
    $cache_result = cache_get($cache_key, 'cache_oauth2');

    if (!isset($cache_result->data)) {
      $result = $this->getVariable('_client')->api($path, $method, $params);
      cache_set($cache_key, $result, 'cache_oauth2', time() + $this->getVariable('_client')->getVariable('expires_in', OAUTH2_DEFAULT_EXPIRES_IN));
      return $result;
    }
    else {
      return $cache_result->data;
    }
  }

  /**
   * Overrides OAuth2::verifyAuthorizedScopes().
   */
  public function verifyAuthorizedScopes($client_id, $required_scope, $uid) {
    return $this->proxyApi('/oauth2/verifyAuthorizedScopes', array(
        'client_id' => $client_id,
        'required_scope' => $required_scope,
        'uid' => $uid,
    ));
  }

  /**
   * Overrides OAuth2::getHiddenScopes().
   */
  public function getHiddenScopes() {
    return $this->proxyApi('/oauth2/getHiddenScopes');
  }

  /**
   * Overrides OAuth2::getAuthorizedScopes().
   */
  public function getAuthorizedScopes($client_id, $uid) {
    return $this->proxyApi('/oauth2/getAuthorizedScopes', array(
        'client_id' => $client_id,
        'uid' => $uid,
    ));
  }

  /**
   * Overrides OAuth2::setAuthorizedScopes().
   */
  public function setAuthorizedScopes($client_id, $scopes, $uid) {
    return $this->proxyApi('/oauth2/setAuthorizedScopes', array(
        'client_id' => $client_id,
        'scopes' => $scopes,
        'uid' => $uid,
    ));
  }

  /**
   * Overrides OAuth2::unsetAuthorizedScopes().
   */
  public function unsetAuthorizedScopes($client_id, $scopes, $uid) {
    return $this->proxyApi('/oauth2/unsetAuthorizedScopes', array(
        'client_id' => $client_id,
        'scopes' => $scopes,
        'uid' => $uid,
    ));
  }

  /**
   * Overrides OAuth2::expireSession().
   */
  public function expireSession($sid) {
    $params = array(
        'sid' => $sid,
        'oauth_token' => $this->getVariable('_client')->getAccessToken(),
    );

    // Purge tokens based on oauth_token if provided.
    if (isset($_REQUEST['oauth_token']) && !empty($_REQUEST['oauth_token'])) {
      $result = $this->getAccessToken($_REQUEST['oauth_token']);
      if ($result) {
        $params = array(
            'sid' => $result['sid'],
            'oauth_token' => $_REQUEST['oauth_token'],
        );
      }
    }

    // Remember to clean cache that out-dated.
    cache_clear_all('*', 'cache_oauth2', TRUE);

    return $this->proxyApi('/oauth2/expireSession', $params);
  }

  /**
   * Implements OAuth2::checkClientCredentials().
   */
  public function checkClientCredentials($client_id, $client_secret = NULL) {
    return $this->proxyApi('/oauth2/checkClientCredentials', array(
        'client_id' => $client_id,
        'client_secret' => $client_secret,
    ));
  }

  /**
   * Implements OAuth2::getRedirectUri().
   */
  public function getRedirectUri($client_id) {
    return $this->proxyApi('/oauth2/getRedirectUri', array(
        'client_id' => $client_id,
    ));
  }

  /**
   * Implements OAuth2::getAccessToken().
   */
  public function getAccessToken($oauth_token) {
    return $this->proxyApi('/oauth2/getAccessToken', array(
        'oauth_token' => $oauth_token,
    ));
  }

  /**
   * Implements OAuth2::setAccessToken().
   */
  public function setAccessToken($oauth_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return $this->proxyApi('/oauth2/setAccessToken', array(
        'oauth_token' => $oauth_token,
        'client_id' => $client_id,
        'expires' => $expires,
        'scope' => $scope,
        'uid' => $uid,
        'sid' => $sid,
    ));
  }

  /**
   * Overrides OAuth2::getSupportedScopes().
   */
  public function getSupportedScopes() {
    return $this->proxyApi('/oauth2/getSupportedScopes');
  }

  /**
   * Overrides OAuth2::getAuthCode().
   */
  public function getAuthCode($code) {
    return $this->proxyApi('/oauth2/getAuthCode', array(
        'code' => $code,
    ));
  }

  /**
   * Overrides OAuth2::setAuthCode().
   */
  public function setAuthCode($code, $client_id, $redirect_uri, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return $this->proxyApi('/oauth2/setAuthCode', array(
        'code' => $code,
        'client_id' => $client_id,
        'redirect_uri' => $redirect_uri,
        'expires' => $expires,
        'scope' => $scope,
        'uid' => $uid,
        'sid' => $sid,
    ));
  }

  /**
   * Overrides OAuth2::checkUserCredentials().
   */
  public function checkUserCredentials($client_id, $username, $password) {
    global $user;

    $result = $this->proxyApi('/oauth2/checkUserCredentials', array(
        'client_id' => $client_id,
        'username' => $username,
        'password' => $password,
    ));

    if ($result) {
      $user = $this->loadUserCredentials($result['uid'], $result['name'], $result['sid']);
    }

    return $result;
  }

  /**
   * Overrides OAuth2::getRefreshToken().
   */
  public function getRefreshToken($refresh_token) {
    return $this->proxyApi('/oauth2/getRefreshToken', array(
        'refresh_token' => $refresh_token,
    ));
  }

  /**
   * Overrides OAuth2::setRefreshToken().
   */
  public function setRefreshToken($refresh_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return $this->proxyApi('/oauth2/setRefreshToken', array(
        'refresh_token' => $refresh_token,
        'client_id' => $client_id,
        'expires' => $expires,
        'scope' => $scope,
        'uid' => $uid,
        'sid' => $sid,
    ));
  }

  /**
   * Overrides OAuth2::unsetRefreshToken().
   */
  public function unsetRefreshToken($refresh_token) {
    return $this->proxyApi('/oauth2/unsetRefreshToken', array(
        'refresh_token' => $refresh_token,
    ));
  }
}
