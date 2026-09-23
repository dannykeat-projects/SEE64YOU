<?php
declare(strict_types=1);

require_once __DIR__ . '/Config.php';
require_once __DIR__ . '/Documents.php';
require_once __DIR__ . '/Auth.php';
require_once __DIR__ . '/Render.php';

$config = s64_config();
$documents = new S64Documents(dirname(__DIR__));
$auth = new S64Auth($config);
