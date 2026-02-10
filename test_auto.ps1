
$server = "127.0.0.1"
$port = 6379

function Send-RedisCommand {
    param(
        [string]$Command
    )
    
    $socket = New-Object System.Net.Sockets.TcpClient($server, $port)
    $stream = $socket.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream, [System.Text.Encoding]::ASCII)
    $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::ASCII)
    $writer.AutoFlush = $true

    # Construct RESP
    $parts = $Command -split " "
    $respCmd = "*" + $parts.Count + "`r`n"
    foreach ($part in $parts) {
        $respCmd += "$" + $part.Length + "`r`n" + $part + "`r`n"
    }

    $writer.Write($respCmd)
    
    # Read response (simple reading logic)
    $line = $reader.ReadLine()
    Write-Host "CMD: $Command" -ForegroundColor Yellow
    Write-Host "RES: $line" -ForegroundColor Cyan
    
    if ($line.StartsWith("$")) {
        $len = [int]($line.Substring(1))
        if ($len -ge 0) {
            Write-Host $reader.ReadLine() -ForegroundColor Cyan
        }
    }
    
    $socket.Close()
}

try {
    Write-Host "--- KvEngine Server Test ---" -ForegroundColor Green
    Send-RedisCommand "PING"
    Send-RedisCommand "SET mykey PowerShellTest"
    Send-RedisCommand "GET mykey"
    Send-RedisCommand "KEYS *"
    Write-Host "--- Test Completed ---" -ForegroundColor Green
} catch {
    Write-Host "Connection failed. Is the server running?" -ForegroundColor Red
}
