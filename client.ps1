
$server = "127.0.0.1"
$port = 6379

try {
    $socket = New-Object System.Net.Sockets.TcpClient($server, $port)
    $stream = $socket.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream, [System.Text.Encoding]::ASCII)
    $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::ASCII)
    $writer.AutoFlush = $true

    Write-Host "已連接到 KvEngine 服務器 ($server:$port)" -ForegroundColor Green
    Write-Host "輸入 Redis 命令 (例如: SET key value, GET key, PING)"
    Write-Host "輸入 'quit' 或 'exit' 退出"

    while ($true) {
        $input = Read-Host "KvEngine>"
        if ([string]::IsNullOrWhiteSpace($input)) { continue }
        if ($input -eq "quit" -or $input -eq "exit") { break }

        # 構造簡單的 RESP 命令
        $parts = $input -split " "
        $respCmd = "*" + $parts.Count + "`r`n"
        foreach ($part in $parts) {
            $respCmd += "$" + $part.Length + "`r`n" + $part + "`r`n"
        }

        # 發送命令
        try {
            $writer.Write($respCmd)
            
            # 讀取響應 (簡單實現: 讀取直到換行或特定字符)
            # 注意: 這裡的實現非常簡單，可能無法處理所有 RESP 響應類型
            # 為了簡單演示，我們假設服務器響應較短且快速
            
            # 嘗試讀取第一行
            $line = $reader.ReadLine()
            if ($null -eq $line) {
                Write-Host "服務器關閉了連接" -ForegroundColor Red
                break
            }
            Write-Host $line
            
            # 如果是 Bulk String ($N)，再讀一行
            if ($line.StartsWith("$")) {
                $len = [int]($line.Substring(1))
                if ($len -ge 0) {
                    $val = $reader.ReadLine()
                    Write-Host $val
                }
            }
            # 如果是 Array (*N)，讀取後續元素
            if ($line.StartsWith("*")) {
                $count = [int]($line.Substring(1))
                for ($i = 0; $i -lt $count; $i++) {
                    $lenLine = $reader.ReadLine() # $len
                    Write-Host $lenLine
                    $val = $reader.ReadLine()     # value
                    Write-Host $val
                }
            }

        } catch {
            Write-Host "發送/接收錯誤: $_" -ForegroundColor Red
            break
        }
    }
} catch {
    Write-Host "無法連接到服務器: $_" -ForegroundColor Red
} finally {
    if ($socket) { $socket.Close() }
}
