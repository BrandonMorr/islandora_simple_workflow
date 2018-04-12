<?php

/**
 * @file
 * Utility functions for use with the Islandora Simple Workflow module.
 */

/**
 * Query for inactive objects.
 *
 * @return array
 *   An array containing the results of the inactive objects query.
 */
function islandora_simple_workflow_get_inactive_objects() {
  $connection = islandora_get_tuque_connection();
  $query = <<<EOQ
SELECT ?object ?title FROM <#ri> WHERE
{
  ?object <fedora-model:label> ?title ;
          <fedora-model:state> <info:fedora/fedora-system:def/model#Inactive>
  !optionals
  !filters
}
ORDER BY ?object
EOQ;
  $optionals = (array) module_invoke('islandora_xacml_api', 'islandora_basic_collection_get_query_optionals', 'view');
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
    '!optionals' => !empty($optionals) ? ('OPTIONAL {{' . implode('} UNION {', $optionals) . '}}') : '',
    '!filters' => !empty($filters) ? implode(' ', array_map($filter_map, $filters)) : '',
  ));
  $results = $connection->repository->ri->sparqlQuery($sparql_query);

  return $results;
}

/**
 * Query for inactive objects parent objects.
 *
 * @param array $inactive_objects
 *   An associative array containing inactive object information.
 */
function islandora_simple_workflow_get_inactive_object_parents(array &$inactive_objects) {
  if (!empty($inactive_objects)) {
    $connection = islandora_get_tuque_connection();
    $query = <<<EOQ
SELECT ?object FROM <#ri>
WHERE {
{
  !child <fedora-rels-ext:isMemberOfCollection> ?object
}
UNION
{
  !child <fedora-rels-ext:isMemberOf> ?object
}
UNION
{
  !child <fedora-rels-ext:isConstituentOf> ?object
}
!optionals
!filters
}
ORDER BY ?object
EOQ;
    $optionals = (array) module_invoke('islandora_xacml_api', 'islandora_basic_collection_get_query_optionals', 'view');
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

    foreach ($inactive_objects as &$inactive_object) {
      if (isset($inactive_object['object']['value']) && !empty($inactive_object['object']['value'])) {
        $sparql_query = format_string($query, array(
          '!child' => '<info:fedora/' . $inactive_object['object']['value'] . '>',
          '!optionals' => !empty($optionals) ? ('OPTIONAL {{' . implode('} UNION {', $optionals) . '}}') : '',
          '!filters' => !empty($filters) ? implode(' ', array_map($filter_map, $filters)) : '',
        ));

        $results = $connection->repository->ri->sparqlQuery($sparql_query);

        if (!empty($results)) {
          $result = reset($results);
          $inactive_object['parent'] = $result['object'];
        }
      }
    }
  }
}
