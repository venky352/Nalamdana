<?php

/**
 * @file
 * OAuth2.0 Scope Identifer administration interface.
 */

/**
 * Administration overview for oauth2_scope.
 */
function oauth2_scope_admin_callback() {
  $header = array(
      array('data' => t('Scope name'), 'field' => 'title'),
      array('data' => t('Scope ID'), 'field' => 'scope_id'),
      array('data' => t('Hidded'), 'field' => 'hidden'),
      array('data' => t('Bypass'), 'field' => 'bypass'),
      array('data' => t('Required'), 'field' => 'required'),
      array('data' => t('Published'), 'field' => 'status'),
      array('data' => t('Updated'), 'field' => 'changed', 'sort' => 'desc'),
      array('data' => t('Operations'), 'colspan' => 2),
  );

  $result = oauth2_scope_load_multiple();

  $output = array();
  $rows = array();

  foreach($result as $scope) {
    $rows[] = array(
        l($scope->title, 'oauth2-scope/'. $scope->scope_id),
        $scope->scope_id,
        $scope->hidden ? t('Yes') : t('No'),
        $scope->bypass ? t('Yes') : t('No'),
        $scope->required ? t('Yes') : t('No'),
        $scope->status ? t('Yes') : t('No'),
        format_interval(time() - $scope->changed),
        l(t('edit'), 'oauth2-scope/'. $scope->scope_id .'/edit', array('query' => array('destination' => 'admin/structure/oauth2/oauth2-scope'))),
        l(t('delete'), 'oauth2-scope/'. $scope->scope_id .'/delete', array('query' => array('destination' => 'admin/structure/oauth2/oauth2-scope'))),
    );
  }
  $output['add_link'] = array(
      '#theme'    => 'link',
      '#text'     => 'Add Scope Identifier',
      '#path'     => 'oauth2-scope/add',
      '#options' => array(
          'attributes' => array('class' => array('oauth2-scope-add-link'), 'id' => 'oauth2-scope-add-link'),
          //REQUIRED:
          'html' => FALSE,
      ),
  );
  $output['scope_list'] = array(
      '#header'  => $header,
      '#rows'    => $rows,
      '#theme'   => 'table',
  );

  return $output;
}

/**
 * Simple page view for 1 row
 */
function oauth2_scope_view_callback($oauth2_scope){
  $header = array(
      array('data' => t('Key'), 'field' => 'Value'),
      array('data' => t('Value'), 'field' => 'Value')
  );
  $rows = array();
  foreach ($oauth2_scope as $key => $value) {
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
 * Add/Edit Scope Identifier form
 *
 * @see oauth2_scope_edit_form_validate()
 * @see oauth2_scope_edit_form_submit()
 */
function oauth2_scope_edit_form($form, &$form_state, $oauth2_scope = null) {

  $form['#oauth2_scope'] = $oauth2_scope;
  $form['oauth2_scope'] = array(
      '#type' => 'fieldset',
      '#title' => t('OAuth2.0 settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
  );

  $form['oauth2_scope']['scope_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Scope ID'),
      '#description' => t('Your OAuth <em>scope_id</em> identifier. Should be alphanumeric strings with hyphens and underscores, 3-32 characters long.'),
      '#default_value' => isset($oauth2_scope->scope_id) ? $oauth2_scope->scope_id : '',
      '#required' => TRUE,
  );

  $form['oauth2_scope']['hidden'] = array(
      '#type' => 'checkbox',
      '#title' => t('Hidden scope'),
      '#description' => t('Boolean indicating whether this scope should include within default empty scope value.'),
      '#default_value' => isset($oauth2_scope->hidden) ? $oauth2_scope->hidden : 0,
  );

  $form['oauth2_scope']['bypass'] = array(
      '#type' => 'checkbox',
      '#title' => t('Bypass end-user authorization'),
      '#description' => t('Boolean indicating whether this scope should bypass end-user authorization.'),
      '#default_value' => isset($oauth2_scope->bypass) ? $oauth2_scope->bypass : 0,
  );

  $form['oauth2_scope']['required'] = array(
      '#type' => 'checkbox',
      '#title' => t('Required scope'),
      '#description' => t('Boolean indicating whether this scope can remove by end-user manually once authorizated for this application.'),
      '#default_value' => isset($oauth2_scope->required) ? $oauth2_scope->required : 0,
  );

  $form['sid'] = array(
      '#type' => 'value',
      '#value' => isset($oauth2_scope->sid) ? $oauth2_scope->sid : NULL,
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
 * Scope Identifier form submit
 */
function oauth2_scope_edit_form_submit($form, &$form_state) {
  $oauth2_scope = array(
      'scope_id'    => $form_state['values']['scope_id'],
      'hidden'      => $form_state['values']['hidden'],
      'bypass'      => $form_state['values']['bypass'],
      'required'    => $form_state['values']['required'],
  );

  if ($form_state['values']['sid'] != NULL) {
    $oauth2_scope['sid'] = $form_state['values']['sid'];
  }
  $return_sid = oauth2_scope_update($oauth2_scope);
  //$form_state['values']['sid'] = $return_sid;

  $form_state['redirect'] = 'oauth2-scope/' . $form_state['values']['scope_id'] .'/edit';
  drupal_set_message($form_state['values']['scope_id'].t(' saved.'));

}

/**
 * Scope Identifer form validate
 */
function oauth2_scope_edit_form_validate($form, &$form_state) {
  $pattern = '/^[a-z0-9-_]{3,64}$/i';
  if (!empty($form_state['values']['scope_id']) && !preg_match($pattern, $form_state['values']['scope_id'])) {
    form_set_error('scope_id', t('Scope ID should be alphanumeric strings with hyphens and underscores, 3-64 characters long.'));
  }

  // Ensure no duplicate scope_id.
  if ($form_state['values']['sid'] != NULL) {
    $result = db_result(db_query("SELECT COUNT(*) AS count FROM {oauth2_scope} AS os  WHERE os.scope_id = :s AND os.sid <> :d", array(':s' => $form_state['values']['scope_id'], ':d' => $form_state['values']['sid'])));
  } else {
    $result = db_result(db_query("SELECT COUNT(*) AS count FROM {oauth2_scope} AS os  WHERE os.scope_id = :s", array(':s' => $form_state['values']['scope_id'])));
  }
  foreach($result as $record) {
    if ($result->count != 0) {
      form_set_error('scope_id', t('This Scope ID had beed used by other scope.'));
    }
  }
}