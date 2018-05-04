<?php

/**
 * @file
 * Utility functions for use with the Islandora Simple Workflow module.
 */

/**
 * Query for inactive objects.
 *
 * @param bool $query_parent
 *   Indicates whether to query parent infomation.
 *
 * @return array
 *   An array containing the results of the inactive objects query.
 */
function islandora_simple_workflow_get_inactive_objects($query_parent = FALSE) {
  $connection = islandora_get_tuque_connection();
  $query = <<<EOQ
SELECT ?object ?title !parent FROM <#ri> WHERE
{
  ?object <fedora-model:label> ?title ;
          <fedora-model:state> <info:fedora/fedora-system:def/model#Inactive>
  !optionals
  !filters
}
ORDER BY ?object
EOQ;
  $optionals = (array) module_invoke('islandora_xacml_api', 'islandora_basic_collection_get_query_optionals', 'view');
  $unions = array(
    '?object <fedora-rels-ext:isMemberOfCollection> ?parent',
    '?object <fedora-rels-ext:isMemberOf> ?parent',
    '?object <fedora-rels-ext:isConstituentOf> ?parent',
  );
  $optionals = array_merge_recursive($optionals, $unions);
  $filter_modules = array(
    'islandora_xacml_api',
    'islandora',
  );
  $filters = array();
  foreach ($filter_modules as $module) {
    $filters = array_merge_recursive($filters, (array) module_invoke($module, 'islandora_basic_collection_get_query_filters', 'view'));
  }
  $filter_map = function ($filter) {
    return "FILTER($filter)";
  };
  $sparql_query = format_string($query, array(
    '!parent' => $query_parent ? '?parent' : '',
    '!optionals' => !empty($optionals) ? ('OPTIONAL {{' . implode('} UNION {', $optionals) . '}}') : '',
    '!filters' => !empty($filters) ? implode(' ', array_map($filter_map, $filters)) : '',
  ));
  $results = $connection->repository->ri->sparqlQuery($sparql_query);

  if ($query_parent) {
    // Check access for each parent object, unset if conditions are not met.
    foreach ($results as &$object) {
      if (isset($object['parent']['value'])) {
        $parent = islandora_object_load($object['parent']['value']);

        if (!$parent || !islandora_object_access(ISLANDORA_VIEW_OBJECTS, $parent)) {
          unset($object['parent']);
        }
      }
    }

    // Check for multiple parents, merge parents into one child array.
    $parsed_results = array();

    foreach ($results as $result) {
      if (!isset($parsed_results[$result['object']['value']])) {
        $parsed_results[$result['object']['value']] = array();
      }
      $parsed_results[$result['object']['value']]['object'] = $result['object'];
      $parsed_results[$result['object']['value']]['title'] = $result['title'];
      $parsed_results[$result['object']['value']]['parent'][] = $result['parent'];
    }

    return $parsed_results;
  }

  return $results;
}
