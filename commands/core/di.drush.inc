<?php
/**
 * @file
 * Commands related to the Dependency Injection container.
 */

/**
 * Implements hook_drush_command().
 */
function di_drush_command() {
  $items = array();

  $items['di-container-dump'] = array(
    'description' => 'Dump the Dependency Injection container to a file.',
    'arguments' => array(
      'path' => 'File path to dump to.',
    ),
    'options' => array(
      'format' => array(
        'description' => 'File format to dump to. If you omit this option, Drush will try to find the format automatically based on the file extension of the specified path.',
        'example-value' => 'xml,yaml,graphviz',
      ),
    ),
    'examples' => array(
      'drush di-container-dump services.xml' => 'Dump the Dependency Injection container to a file called services.xml in the current directory.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'required-arguments' => TRUE,
    'aliases' => array(
      'di-dump',
    ),
    'core' => array('8+'),
  );

  return $items;
}

/**
 * Command callback. Dump the dependency injection container to a file.
 */
function drush_di_container_dump($path) {
  // We'd rather go the OO way instead of using the Drupal static class, because
  // Drupal::getContainer() is deprecated.
  $kernel = new \Drupal\Core\DrupalKernel('prod', drupal_classloader());
  $kernel->boot();

  $format = drush_get_option('format');

  if (!$format) {
    // Coudn't use file_get_mimetype() here because it does not know
    // .yml / a yaml MIME type and thinks .dot files are msword instead of
    // graphviz.
    $file_extension = pathinfo($path, PATHINFO_EXTENSION);

    switch ($file_extension) {
      case 'dot':
        $format = 'graphviz';
        break;

      case 'yml':
        $format = 'yaml';
        break;

      default:
        $format = $file_extension;
    }
  }

  $container = $kernel->getContainer();

  switch ($format) {
    case 'yaml':
      $dumper = new Symfony\Component\DependencyInjection\Dumper\YamlDumper($container);
      break;

    case 'php':
      $dumper = new Symfony\Component\DependencyInjection\Dumper\PhpDumper($container);
      break;

    case 'graphviz':
      $dumper = new Symfony\Component\DependencyInjection\Dumper\GraphvizDumper($container);
      break;

    case 'xml':
      $dumper = new Symfony\Component\DependencyInjection\Dumper\XmlDumper($container);
      break;

    default:
      return drush_set_error('DRUSH_DI_UNSUPPORTED_FORMAT', dt('Unsupported format !format.', array('!format' => $format)));
  }

  try {
    $dump = $dumper->dump();
  }
  catch (Exception $e) {
    return drush_set_error('DRUSH_DI_DUMPER_EXCEPTION', dt('An exception occurred while dumping the DI container. Message: !message', array('!message' => $e->getMessage())));
  }

  // @todo Error handling for inaccessible files etc.
  if (!is_writable($path)) {
    return drush_set_error('DRUSH_DI_PATH_NOT_WRITABLE', dt('Unable to write to file !file.', array('!file' => $path)));
  }

  file_put_contents($path, $dump);

  return dt('Dependency Injection container dumped in !format to !file', array('!format' => $format, '!file' => $path));
}
