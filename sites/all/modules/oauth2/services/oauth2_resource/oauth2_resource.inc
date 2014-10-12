<?php

/**
 * @file
 *  Link general user functionalities to services module.
 */

function oauth2_resource_verify_authorized_scopes($scope_data) {
  return oauth2_get_server()->verifyAuthorizedScopes($scope_data['client_id'], $scope_data['required_scope'], $scope_data['uid']);
}

function oauth2_resource_get_hidden_scopes() {
  return oauth2_get_server()->getHiddenScopes();
}

function oauth2_resource_get_authorized_scopes($scope_data) {
  return oauth2_get_server()->getAuthorizedScopes($scope_data['client_id'], $scope_data['uid']);
}

function oauth2_resource_set_authorized_scopes($scope_data) {
  return oauth2_get_server()->setAuthorizedScopes($scope_data['client_id'], $scope_data['scopes'], $scope_data['uid']);
}

function oauth2_resource_unset_authorized_scopes($scope_data) {
  return oauth2_get_server()->unsetAuthorizedScopes($scope_data['client_id'], $scope_data['scopes'], $scope_data['uid']);
}

function oauth2_resource_expire_session($sid_array) {
  return oauth2_get_server()->expireSession($sid_array['sid']);
}

function oauth2_resource_check_client_credentials($client_data) {
  if (!isset($client_data['client_secret'])) {
    $client_data['client_secret'] = NULL;
  }
  return oauth2_get_server()->checkClientCredentials($client_data['client_id'], $client_data['client_secret']);
}

function oauth2_resource_get_redirect_uri($client_data) {
  return oauth2_get_server()->getRedirectUri($client_data['client_id']);
}

function oauth2_resource_get_access_token($oauth_data) {
  if (!isset($oauth_data['access_token']) && isset($oauth_data['oauth_token'])) {
    $oauth_data['access_token'] = $oauth_data['oauth_token'];
  }
  return oauth2_get_server()->getAccessToken($oauth_data['access_token']);
}

function oauth2_resource_set_access_token($oauth_data) {
  if (!isset($oauth_data['scope'])) {
    $oauth_data['scope'] = '';
  }
  if (!isset($oauth_data['uid'])) {
    $oauth_data['uid'] = 0;
  }
  if (!isset($oauth_data['sid'])) {
    $oauth_data['sid'] = '';
  }
  if (!isset($oauth_data['access_token']) && isset($oauth_data['oauth_token'])) {
    $oauth_data['access_token'] = $oauth_data['oauth_token'];
  }
  return oauth2_get_server()->setAccessToken($oauth_data['access_token'], $oauth_data['client_id'], $oauth_data['expires'], $oauth_data['scope'], $$oauth_data['uid'], $oauth_data['sid']);
}

function oauth2_resource_get_supported_scopes() {
  return oauth2_get_server()->getSupportedScopes();
}

function oauth2_resource_get_auth_code($code_array) {
  return oauth2_get_server()->getAuthCode($code_array['code']);
}

function oauth2_resource_set_auth_code($oauth_data) {
  if (!isset($oauth_data['scope'])) {
    $oauth_data['scope'] = '';
  }
  if (!isset($oauth_data['uid'])) {
    $oauth_data['uid'] = 0;
  }
  if (!isset($oauth_data['sid'])) {
    $oauth_data['sid'] = '';
  }
  return oauth2_get_server()->setAuthCode($oauth_data['code'], $oauth_data['client_id'], $oauth_data['redirect_uri'], $oauth_data['expires'], $oauth_data['scope'], $oauth_data['uid'], $oauth_data['sid']);
}

function oauth2_resource_check_user_credentials($user_data) {
  return oauth2_get_server()->checkUserCredentials($user_data['client_id'], $user_data['username'], $user_data['password']);
}

function oauth2_resource_get_refresh_token($refresh_token_array) {
  return oauth2_get_server()->getRefreshToken($refresh_token_array['refresh_token']);
}

function oauth2_resource_set_refresh_token($oauth_data) {
  if (!isset($oauth_data['scope'])) {
    $oauth_data['scope'] = '';
  }
  if (!isset($oauth_data['uid'])) {
    $oauth_data['uid'] = 0;
  }
  if (!isset($oauth_data['sid'])) {
    $oauth_data['sid'] = '';
  }
  return oauth2_get_server()->setRefreshToken($oauth_data['refresh_token'], $oauth_data['client_id'], $oauth_data['expires'], $oauth_data['scope'], $oauth_data['uid'], $oauth_data['sid']);
}

function oauth2_resource_unset_refresh_token($refresh_token_array) {
  return oauth2_get_server()->unsetRefreshToken($refresh_token_array['refresh_token']);
}

function oauth2_resource_retrieve() {
  return 'nothing to retrieve';
}
