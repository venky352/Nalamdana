<?php

/**
 * @file
 * OAuth2.0 Server Identifer administration interface.
 */

/**
 * Administration overview for oauth2_server.
 */
function oauth2_server_admin_callback() {
  $header = array(
      array('data' => t('Server Name'), 'field' => 'title'),
      array('data' => t('Server ID'), 'field' => 'server_id'),
      array('data' => t('Updated'), 'field' => 'changed', 'sort' => 'desc'),
      array('data' => t('Operations'), 'colspan' => 2),
  );

  /*$query = new EntityFieldQuery();
  $query
  ->entityCondition('entity_type', 'oauth2_server', '=');
  $result = $query->execute();
  $sids = array();
  foreach($result['oauth2_server'] as $record) {
    $sids[] = $record->sid;
  }
  $result = oauth2_server_load_multiple($sids);*/
  
 
  $result = oauth2_server_load_multiple();
  
  
  //$result = db_query("SELECT osrv.sid, osrv.title, osrv.server_id, osrv.status, osrv.changed FROM {oauth2_server} AS osrv ");
  //$result = oauth2_server_load_multiple();
  //error_log(print_r($temp,true));
  $output = array();
  $rows = array();
  $output['add_link'] = array(
      '#theme'    => 'link',
      '#text'     => 'Add Server Identifier',
      '#path'     => 'admin/structure/oauth2/oauth2-server/add',
      '#options' => array(
          'attributes' => array('class' => array('oauth2-server-add-link'), 'id' => 'oauth2-server-add-link'),
          //REQUIRED:
          'html' => FALSE,
      ),
  );
  $output['server_list'] = array();
  $output['server_list']['#header'] = $header;
  foreach($result as $server) {
    $rows[] = array(
        l($server->title, 'admin/structure/oauth2/oauth2-server/'. $server->server_id),
        $server->server_id,
        format_interval(time() - $server->changed),
        l(t('edit'), 'admin/structure/oauth2/oauth2-server/'. $server->server_id .'/edit', array('query' => array('destination' => 'admin/structure/oauth2/oauth2-server'))),
        l(t('delete'), 'admin/structure/oauth2/oauth2-server/'. $server->server_id .'/delete', array('query' => array('destination' => 'admin/structure/oauth2/oauth2-server'))),
    );
  }
  $output['server_list']['#rows'] = $rows;
  $output['server_list']['#theme'] = 'table';


  /*if (empty($rows)) {
   $rows[] = array(array('data' => '<em>'. t('There are currently no OAuth2.0 Server Identifer.') .'</em>', 'colspan' => 6));
  }*/
  return $output;

}


/**
 * Simple page view for 1 row
 */
function oauth2_server_view_callback($oauth2_server) {
  $header = array(
      array('data' => t('Key'), 'field' => 'Value'),
      array('data' => t('Value'), 'field' => 'Value')
  );
  $rows = array();
  foreach ($oauth2_server as $key => $value) {
    if ($key == 'sid') {
      continue;
    }
    $rows[] = array($key,$value);
  }
  $output = array(
      '#header' => $header,
      '#rows' => $rows,
      '#theme' => 'table'
  );
  return $output;
}

/**
 * Add/Edit Server Identifier form
 *
 * @see oauth2_server_edit_form_validate()
 * @see oauth2_server_edit_form_submit()
 */
function oauth2_server_edit_form($form, &$form_state, $oauth2_server = NULL) {

  $form['#oauth2_server'] = $oauth2_server;
  $form['oauth2_server'] = array(
      '#type' => 'fieldset',
      '#title' => t('OAuth2.0 settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
  );
  
  $form['oauth2_server']['server_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Server ID'),
      '#description' => t('The OAuth2.0 machine name<em>(server_id)</em>.'),
      '#default_value' => isset($oauth2_server->server_id) ? $oauth2_server->server_id : '',
      '#required' => TRUE,
      '#weight' => -6,
  );
  
  $form['oauth2_server']['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#required' => TRUE,
      '#default_value' => isset($oauth2_server->title) ? $oauth2_server->title : '',
      '#maxlength' => 255,
      '#weight' => -5,
  );



  $form['oauth2_server']['base_uri'] = array(
      '#type' => 'textfield',
      '#title' => t('Base URI'),
      '#description' => t('The authorization server\'s HTTP base URI, typically to the front page.'),
      '#default_value' => isset($oauth2_server->base_uri) ? $oauth2_server->base_uri : '',
      '#required' => TRUE,
  );

  $form['oauth2_server']['services_uri'] = array(
      '#type' => 'textfield',
      '#title' => t('Services endpoint'),
      '#description' => t('The remote server\'s HTTP endpoint capable of Drupal Service 3.x services. Absolute path or path relative to base URI.'),
      '#default_value' => isset($oauth2_server->services_uri) ? $oauth2_server->services_uri : '',
      '#required' => FALSE,
  );

  $form['oauth2_server']['client_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Application ID'),
      '#description' => t('Your registered OAuth2.0 <em>client_id</em>.'),
      '#default_value' => isset($oauth2_server->client_id) ? $oauth2_server->client_id : '',
      '#required' => FALSE,
  );

  $form['oauth2_server']['client_secret'] = array(
      '#type' => 'textfield',
      '#title' => t('Application Secret'),
      '#description' => t('Your registered OAuth2.0 <em>client_secret</em>.'),
      '#default_value' => isset($oauth2_server->client_secret) ? $oauth2_server->client_secret : '',
      '#required' => FALSE,
  );

  $form['oauth2_server']['authorize_uri'] = array(
      '#type' => 'textfield',
      '#title' => t('End-user authorization endpoint'),
      '#description' => t('The authorization server\'s HTTP endpoint capable of authenticating the end-user and obtaining authorization. Absolute path or path relative to base URI.'),
      '#default_value' => isset($oauth2_server->authorize_uri) ? $oauth2_server->authorize_uri : 'oauth2/authorize',
      '#required' => FALSE,
  );

  $form['oauth2_server']['access_token_uri'] = array(
      '#type' => 'textfield',
      '#title' => t('Token endpoint'),
      '#description' => t('The authorization server\'s HTTP endpoint capable of issuing tokens and refreshing expired tokens. Absolute path or path relative to base URI.'),
      '#default_value' => isset($oauth2_server->access_token_uri) ? $oauth2_server->access_token_uri : 'oauth2/access_token',
      '#required' => FALSE,
  );

  $form['sid'] = array(
      '#type' => 'value',
      '#value' => isset($oauth2_server->sid) ? $oauth2_server->sid : NULL,
  );

  $form['actions'] = array(
      '#type' => 'actions',
      'save' => array(
          '#type' => 'submit',
          '#value' => t('Save settings'),
      )
  );


  return $form;

}

/**
 * Server Identifier form submit
 *
 * @see oauth2_server_edit_form()
 */
function oauth2_server_edit_form_submit($form, &$form_state) {
  $oauth2_server =  array(
      'server_id'         => $form_state['values']['server_id'],
      'base_uri'          => $form_state['values']['base_uri'],
      'services_uri'      => $form_state['values']['services_uri'],
      'client_id'         => $form_state['values']['client_id'],
      'client_secret'     => $form_state['values']['client_secret'],
      'authorize_uri'     => $form_state['values']['authorize_uri'],
      'access_token_uri'  => $form_state['values']['access_token_uri'],
      'title'             => $form_state['values']['title'],
  );
  if ($form_state['values']['sid'] != NULL) {
    $oauth2_server['sid'] = $form_state['values']['sid'];
  }
  oauth2_server_update($oauth2_server);
  //$form_state['values']['sid'] = $return_sid;

  $form_state['redirect'] = 'admin/structure/oauth2/oauth2-server/' . $form_state['values']['server_id'] . '/edit';
  drupal_set_message($form_state['values']['title'].t(' saved.'));
}

/**
 * Server Identifier form validate
 *
 * @see oauth2_server_edit_form()
 */
function oauth2_server_edit_form_validate($form, &$form_state) {

  $pattern = '/^[a-z0-9-_]{3,64}$/i';
  if (!empty($form_state['values']['server_id']) && !preg_match($pattern, $form_state['values']['server_id'])) {
    form_set_error('server_id', t('Server ID should be alphanumeric strings with hyphens and underscores, 3-64 characters long.'));
  }

  // Ensure no duplicate server_id.
  if ($form_state['values']['sid'] != NULL) {
    $result = db_query("SELECT COUNT(*) AS count FROM {oauth2_server} AS osrv  WHERE osrv.server_id = :s AND osrv.sid != :t ", array(':s' => $form_state['values']['server_id'],':t' => $form_state['values']['sid']));
  } else {
    $result = db_query("SELECT COUNT(*) AS count FROM {oauth2_server} AS osrv  WHERE osrv.server_id = :s ", array(':s' => $form_state['values']['server_id']));
  }
  foreach($result as $record)
    if ($record->count != 0) {
    form_set_error('server_id', t('This Server ID had beed used by other server identifer.'));
  }
}

/**
 * Confirm form for the deletion
 */
function oauth2_server_delete_form($form, &$form_state, $oauth2_server) {
  $destination = drupal_get_destination();

  //confirm_form($form, $question, $path, $description = NULL, $yes = NULL, $no = NULL, $name = 'confirm')
  return confirm_form(
  array(
      'oauth2_server' => array(
          '#type' => 'value',
          '#value' => $oauth2_server->server_id,
      ),
  ),
  t('Are you sure you want to delete this Server Identifier?'),
  $destination['destination'],
  t('This action cannot be undone.'),
  t('Delete Server Identifier'),
  t('Cancel')
  );
}
/**
 * Confirm form submit for the deletion
 */
function oauth2_server_delete_form_submit($form, &$form_state) {
  oauth2_server_delete($form_state['values']['oauth2_server']);
  drupal_set_message(t('Server Identifier deleted'));
}
