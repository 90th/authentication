<?php
// Define the path to the files
$licenseKeysFilePath = "licensekeys.txt";
$licensesFilePath = "licenses.txt";
$apiKeysFilePath = "apikeys.txt";

// Hard-coded program hash
$programHash = "TEST";


// Define rate limit settings
$rateLimitWindow = 60; // Time window in seconds
$rateLimitMaxRequests = 10; // Maximum number of requests per time window

// Get client IP address
$clientIP = $_SERVER["REMOTE_ADDR"];

// Calculate current time window
$currentWindow = time() - ($rateLimitWindow - 1);

// Count requests made in the current time window
$requestCount = 0;
$requestLog = file("ratelimit_log.txt", FILE_IGNORE_NEW_LINES);
foreach ($requestLog as $entry) {
    list($timestamp, $ip) = explode(",", $entry);
    if ($ip === $clientIP && $timestamp >= $currentWindow) {
        $requestCount++;
    }
}

if ($requestCount >= $rateLimitMaxRequests) {
    echo "Rate limit exceeded";
    exit;
}



// Log the current request
$currentRequestLog = time() . "," . $clientIP . "\n";
file_put_contents("ratelimit_log.txt", $currentRequestLog, FILE_APPEND);


// Get parameters from the request
$licenseKey = $_GET["licenseKey"];
$hwid = $_GET["hwid"];
$apiKeyParam = $_GET["apiKey"];
$programHashParam = $_GET["programHash"];

// Check if the provided API key matches the stored API keys
$authorized = false;
$apiKeys = file($apiKeysFilePath, FILE_IGNORE_NEW_LINES);
foreach ($apiKeys as $storedApiKey) {
    if ($apiKeyParam === $storedApiKey) {
        $authorized = true;
        break;
    }
}

if (!$authorized) {
    echo "Authorization failed";
    exit;
}

if ($programHashParam !== $programHash) {
    echo "Program hash mismatch";
    exit;
}

// Check if the license key has already been claimed
$licenses = file($licensesFilePath, FILE_IGNORE_NEW_LINES);
if (in_array("$licenseKey:$hwid", $licenses)) {
    echo "OK";
} else {
    // If not claimed, check if the license key exists in the license keys file
    $licenseKeys = file($licenseKeysFilePath, FILE_IGNORE_NEW_LINES);
    if (in_array($licenseKey, $licenseKeys)) {
        // Remove the license key from the license keys file
        $licenseKeys = array_diff($licenseKeys, array($licenseKey));
        file_put_contents($licenseKeysFilePath, implode(PHP_EOL, $licenseKeys));

        // Add the license key + hwid to the licenses file
        $licenses[] = "$licenseKey:$hwid";
        file_put_contents($licensesFilePath, implode(PHP_EOL, $licenses));

        echo "License claimed";
    } else {
        echo "License not found";
    }
}
?>
