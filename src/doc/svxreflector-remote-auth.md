# SvxReflector remote user authentication

When `[REMOTE_USER_AUTH]` / `USER_AUTH_ENABLE=1` is set in `svxreflector.conf`, the reflector will ask a **remote HTTP service** whether this login should be allowed, if it's not able to contact that service, or if the service replies negatively, it'll fallback to the usual local verification using the credentials defined in `[USERS]` / `[PASSWORDS]`.

## Order of checks during login

1. If remote auth is enabled and configured, the reflector POSTs `username`, `digest`, and `challenge` (see below) to `USER_AUTH_URL` with a `Bearer` token (used to authenticate svxreflector with the authentication service).
2. If the JSON response has `"success": true`, the client is accepted.
3. If remote auth fails (network error, HTTP error, `success` not true, invalid JSON, etc.), the reflector **falls back** to the usual local verification.

## Cryptography (what your server must implement)

The node’s digest is the same as the reflector’s local check (`MsgAuthResponse` in `ReflectorMsg.h`):

- **Algorithm:** HMAC-SHA1.
- **HMAC key:** the user’s authentication string (the “password” / shared secret from your account system), treated as raw bytes (same as in `svxreflector.conf` `[PASSWORDS]`).
- **HMAC message:** the 20-byte **challenge** (binary), not the hex string.

The JSON fields sent to your URL are **hex-encoded** (lowercase in practice, but you should accept any hex case):

| Field       | Meaning |
|------------|---------|
| `username` | Callsign / user id (string). |
| `challenge`| 40 hex chars = 20 bytes of random challenge originally sent by the reflector. |
| `digest`   | 40 hex chars = 20-byte HMAC-SHA1 output produced by the **node** using its copy of the password. |

Your service should decode `challenge` from hex, look up the password for `username`, compute `HMAC-SHA1(password, challenge_binary)`, hex-encode the result, and compare in a **constant-time** manner to `digest`. If they match and the account is allowed, return `"success": true`.

## HTTP API

**Request**

- Method: `POST`
- Header: `Authorization: Bearer <USER_AUTH_TOKEN>` (same token as in `svxreflector.conf`).
- Header: `Content-Type: application/json`
- Body example:

```json
{
  "username": "SM0ABC",
  "digest": "a1b2c3d4e5f6789012345678901234567890abcd",
  "challenge": "fedcba0987654321fedcba0987654321fedcba09"
}
```

**Response**

- `Content-Type: application/json`
- HTTP status in the 2xx range for a well-formed decision (the reflector treats non-2xx as failure).
- Body must be JSON with at least:
  - `"success":` boolean — only explicit `true` is treated as success.
  - `"message":` string — logged / shown for debugging.

Example success:

```json
{
  "success": true,
  "message": "OK"
}
```

Example rejection:

```json
{
  "success": false,
  "message": "Account disabled"
}
```

TLS certificate verification on the reflector side is controlled by `USER_AUTH_FORCE_VALID_SSL`.

---

## Minimal PHP example

Below is a single-file illustration (not production-hardened). It shows bearer validation and HMAC-SHA1 verification matching the reflector protocol.

```php
<?php
/**
 * Minimal remote auth endpoint for SvxReflector.
 * Point USER_AUTH_URL to this script (HTTPS recommended).
 * Set USER_AUTH_TOKEN in svxreflector.conf to the same value as BEARER_SECRET.
 */
declare(strict_types=1);

// Same secret as USER_AUTH_TOKEN in [REMOTE_USER_AUTH]
const BEARER_SECRET = 'your_secret_token_here';

// Example user database: callsign => shared secret (same idea as [PASSWORDS])
const USERS = [
    'SM0ABC' => 'MyNodes',
    'SM3XYZ' => 'A strong password',
];

header('Content-Type: application/json; charset=utf-8');

$auth = $_SERVER['HTTP_AUTHORIZATION'] ?? '';
if (!preg_match('/^Bearer\s+(\S+)/i', $auth, $m)) {
    http_response_code(401);
    echo json_encode(['success' => false, 'message' => 'Missing or invalid Authorization']);
    exit;
}
if (!hash_equals(BEARER_SECRET, $m[1])) {
    http_response_code(401);
    echo json_encode(['success' => false, 'message' => 'Invalid token']);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['success' => false, 'message' => 'POST required']);
    exit;
}

$payload = json_decode(file_get_contents('php://input') ?: '', true);
if (!is_array($payload)) {
    http_response_code(400);
    echo json_encode(['success' => false, 'message' => 'Invalid JSON']);
    exit;
}

foreach (['username', 'digest', 'challenge'] as $k) {
    if (!isset($payload[$k]) || !is_string($payload[$k])) {
        http_response_code(400);
        echo json_encode(['success' => false, 'message' => "Missing field: $k"]);
        exit;
    }
}

$username = $payload['username'];
$digestHex = strtolower(preg_replace('/[^0-9a-f]/', '', $payload['digest']));
$challengeHex = strtolower(preg_replace('/[^0-9a-f]/', '', $payload['challenge']));

if (strlen($digestHex) !== 40 || strlen($challengeHex) !== 40) {
    echo json_encode(['success' => false, 'message' => 'digest/challenge must be 40 hex chars (20 bytes)']);
    exit;
}

$challengeBin = hex2bin($challengeHex);
if ($challengeBin === false || strlen($challengeBin) !== 20) {
    echo json_encode(['success' => false, 'message' => 'invalid challenge hex']);
    exit;
}

if (!array_key_exists($username, USERS)) {
    echo json_encode(['success' => false, 'message' => 'Unknown user']);
    exit;
}

$password = USERS[$username];
$expected = hash_hmac('sha1', $challengeBin, $password, false); // hex string, lowercase

if (!hash_equals($expected, $digestHex)) {
    echo json_encode(['success' => false, 'message' => 'Digest mismatch']);
    exit;
}

echo json_encode([
    'success' => true,
    'message' => 'Authentication successful',
]);
```

Replace `USERS` with your own lookup (SQL, LDAP, etc.). 
