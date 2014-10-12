<?php

/**
 * @file
 * Drupal OAuth2.0 client library.
 */

/**
 * Provides access to the Drupal OAuth2.0 Platform.
 *
 * @author Edison Wong <hswong3i@pantarei-design.com>
 */
class DrupalOAuth2Client extends OAuth2Client {

  /**
   * Overrides OAuth2Client::__construct().
   */
  public function __construct($config = array()) {
    global $user;

    parent::__construct($config);

    $oauth2_mode = variable_get('oauth2_provider_mode');
    //only for client mode and no need to load if already logged in
    if ($oauth2_mode == 'DrupalOAuth2Client' && !user_is_logged_in()) {

      // devel_execute_block_form also submit execute code in $_REQUEST['code'].
      if (!isset($_REQUEST['form_id']) || $_REQUEST['form_id'] !== 'devel_execute_block_form') {

        $access_token = $this->getAccessToken();

        if (!empty($access_token)) {
          $result = $this->api('/oauth2/getAccessToken', 'POST', array(
              'oauth_token' => $access_token,
          )); //get the user data using access token
          //error_log(print_r($result,true));
          $_SESSION['oauth2_temp_user_profile'] = $result;
          if ($result) {
            $this->loadUserCredentials($result['name'], $result['mail']);
          }
        }
      }

    }
  }
  /**
   * Overrides OAuth2Client::api()
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
  public function api($path, $method = 'GET', $params = array()) {
    if (is_array($method) && empty($params)) {
      $params = $method;
      $method = 'GET';
    }

    // json_encode all params values that are not strings.
    foreach ($params as $key => $value) {
      if (!is_string($value)) {
        $params[$key] = json_encode($value);
      }
    }

    $result = json_decode($this->makeOAuth2Request(
    $this->getUri($path),
    $method,
    $params
    ), TRUE);

    // Results are returned, errors are thrown.
    if (is_array($result) && isset($result['error'])) {
      $e = new OAuth2Exception($result);
      $error_type = $e->getType();
      switch ($error_type) {
        // OAuth 2.0 Draft 10 style.
        case OAUTH2_ERROR_EXPIRED_TOKEN:

          break;
        case OAUTH2_ERROR_INVALID_TOKEN:
          //$this->setSession(NULL);
          $this->setSession(NULL);
          user_logout();
          break;
        default:
          //special hack..coz the library is bugged
          if (strpos($error_type, OAUTH2_ERROR_EXPIRED_TOKEN) === 0) {
            $access_token = $this->getAccessTokenFromRefreshToken();
            //error_log('new access_token '.print_r($access_token,true));
            $session = $this->getSessionObject($access_token);
            $session = $this->validateSessionObject($session);
            if (!empty($session)) {
              //if session is valid
              $this->setSession($session);

              //update the access token in $params
              if (isset($params['oauth_token'])) {
                $params['oauth_token'] = $this->getAccessToken();
              } elseif (isset($params['access_token'])) {
                $params['access_token'] = $this->getAccessToken();
              }
              //try to call api again
              $result = json_decode($this->makeOAuth2Request(
              $this->getUri($path),
              $method,
              $params
              ), TRUE);

              if (is_array($result) && isset($result['error'])) {
                //if still failed
                $this->setSession(NULL);
                user_logout();
              } else {
                return $result;
              }
            }else {
              //if still invalid
              $this->setSession(NULL);
              user_logout();
            }
            break;
          } else if (strpos($error_type, OAUTH2_ERROR_INVALID_TOKEN) === 0) {
            $this->setSession(NULL);
            user_logout();
            break;
          }


          $this->setSession(NULL);
          throw $e;
      }

    }
    return $result;
  }
  /**
   *
   */
  public function getAccessTokenFromRefreshToken() {

    if ($this->getVariable('access_token_uri') && $this->getVariable('client_id') && $this->getVariable('client_secret')) {
      return json_decode($this->makeRequest(
      $this->getVariable('access_token_uri'),
      'POST',
      array(
          'grant_type' => OAUTH2_GRANT_TYPE_REFRESH_TOKEN,
          'refresh_token' => $this->getRefreshToken(),
          'client_id' => $this->getVariable('client_id'),
          'client_secret' => $this->getVariable('client_secret'),
      )
      ), TRUE);
    }
    return NULL;
  }

  /**
   * @return
   *  The valid OAuth2 refresh token, or NULL if not exist
   */
  public function getRefreshToken() {
    $session = $this->getSession();
    return isset($session['refresh_token']) ? $session['refresh_token'] : NULL;
  }
  /**
   * Expires all OAuth2.0 related tokens based on sid.
   *
   * @return
   *   TRUE if successful, and FALSE if it isn't.
   */
  public function expireSession() {

    // we won't know the server session id
    $params = array(
        'sid' => NULL,
        'oauth_token' => $this->getAccessToken(),
    );

    // Purge tokens based on oauth_token if provided.
    /*if (isset($_REQUEST['oauth_token']) && !empty($_REQUEST['oauth_token'])) {
    $result = $this->getAccessToken($_REQUEST['oauth_token']);
    if ($result) {
    $params = array(
        'sid' => $result['sid'],
        'oauth_token' => $_REQUEST['oauth_token'],
    );
    }
    }*/


    return $this->api('/oauth2/expireSession', 'POST', $params);
  }
  /**
   * Load the user based on uid and name, assign with corresponding session;
   * or create dummy user on-the-fly.
   *
   * @param $name
   *   The user name.
   * @param $mail
   *   The user email.
   *
   * @return
   *   The user object after successful login.
   */
  protected function loadUserCredentials($name, $mail) {
    global $user;

    //root is not allowed
    $root_user = user_load(1);
    if ($name == $root_user->name) {
      return $user;
    }

    //check if exists in authmap
    $account = user_external_load($name);
    if (!$account) {
      //try to load user from existing user table
      $account = user_load_by_name($name);

      if ($account !== FALSE) {
        //if user exist
        user_set_authmaps($account, array("authname_oauth2" => $name));
        $temp_form_state = array('uid' => $account->uid);
        user_login_submit(array(), $temp_form_state);
      } else {
        //error_log('create user: '.$name);
        //user_external_login_register($name,'oauth2');


        // Register this new user.
        $userinfo = array(
            'name' => $name,
            'pass' => user_password(),
            'init' => $name,
            'mail' => $name.'@localhost.localdomain',
            'status' => 1,
            'access' => REQUEST_TIME
        );

        $account = user_save(drupal_anonymous_user(), $userinfo);
        // Terminate if an error occurred during user_save().
        if (!$account) {
          drupal_set_message(t("Error saving user account."), 'error');
          return;
        }

        user_set_authmaps($account, array("authname_oauth2" => $name));


        // Log user in.
        $form_state['uid'] = $account->uid;
        user_login_submit(array(), $form_state);


        //user_save($user,array('mail' => $name.'@localhost.localdomain'));

      }
    } else {
      //load it

      $temp_form_state = array('uid' => $account->uid);
      user_login_submit(array(), $temp_form_state);
    }
    drupal_goto($_GET['q']);
    return $account;
  }
  /**
   * Get a Login URL for use with redirects. By default, full page redirect is
   * assumed. If you are using the generated URL with a window.open() call in
   * JavaScript, you can pass in display = popup as part of the $params.
   *
   * @param $params
   *   Provide custom parameters.
   *
   * @return
   *   The URL for the login flow.
   */
  public function getLoginUri($params = array()) {
    return $this->getUri(
    $this->getVariable('base_uri') . 'user',
    array_merge(array(
        'response_type' => 'code',
        'client_id' => $this->getVariable('client_id'),
        'redirect_uri' => $this->getCurrentUri(),
    ), $params)
    );
  }

  public function getLoginUriRedirect($redirect_uri = '',$params = array()) {
    if ($redirect_uri == '') {
      $redirect_uri = $this->getCurrentUri();
    }
    return $this->getUri(
    $this->getVariable('base_uri') . 'user',
    array_merge(array(
        'response_type' => 'code',
        'client_id' => $this->getVariable('client_id'),
        'redirect_uri' => $redirect_uri,
    ), $params)
    );
  }

  /**
   * Get a Logout URL suitable for use with redirects.
   *
   * @param $params
   *   Provide custom parameters.
   *
   * @return
   *   The URL for the logout flow.
   */
  public function getLogoutUri($params = array()) {
    return $this->getUri(
    $this->getVariable('base_uri') . 'logout',
    array_merge(array(
        'oauth_token' => $this->getAccessToken(),
        'redirect_uri' => $this->getCurrentUri(),
    ), $params)
    );
  }
}
