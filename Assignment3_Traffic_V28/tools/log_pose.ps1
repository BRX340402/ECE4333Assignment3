<#
 log_pose.ps1 - log the robot's predicted / fused pose to CSV while it drives the figure-8.

 1. Power the car, connect the PC Wi-Fi to the car's access point (same as for the web page).
 2. powershell -ExecutionPolicy Bypass -File tools\log_pose.ps1            (Ctrl+C to stop)
    options:  -Out run1.csv   -Url http://192.168.4.1/traffic-status   -Seconds 150
 3. Press Start on the web page.  One CSV row is written per UNO telemetry packet
    (every 0.4 s, de-duplicated with the UNO uptime counter UT).
 4. Plot:  powershell -File tools\plot_pose.ps1 -In run1.csv     (Windows PowerShell 5.1)
           or  python tools\plot_pose.py run1.csv                  (matplotlib)
 Units: x,y in cm (origin = start point, +x = initial heading, +y = left), headings in degrees
 (+ = counter-clockwise).  UNO sends H/KH in 0.1 deg, converted here.
#>
param(
 [string]$Url="http://192.168.4.1/traffic-status",
 [string]$Out="pose_log.csv",
 [int]$PollMs=200,
 [int]$Seconds=0
)
function Wrap180([double]$a){ while($a -gt 180){$a-=360}; while($a -lt -180){$a+=360}; return $a }
$cols="pc_time,t_s,mode,state,seg,lap,phase,fused_x_cm,fused_y_cm,fused_heading_deg,pred_x_cm,pred_y_cm,pred_heading_deg,gyro_yaw_deg,gyro_heading_wrapped_deg,gyro_ok,progress_cm,event_err_cm,speed_scale,fault,fault_from_state,line_L,line_C,line_R,landmark_corrected"
Set-Content -Path $Out -Value $cols -Encoding ASCII
$lastUT=-1; $t0=$null; $rows=0; $started=Get-Date
Write-Host "Logging $Url -> $Out  (Ctrl+C to stop)"
try{
 while($true){
  if($Seconds -gt 0 -and ((Get-Date)-$started).TotalSeconds -gt $Seconds){break}
  try{
   $d=Invoke-RestMethod -Uri $Url -TimeoutSec 2
   $u=$d.uno
   if($u -and $u.UT -ne $null -and $u.UT -ne $lastUT){
    $lastUT=$u.UT; if($t0 -eq $null){$t0=$u.UT}
    $t=[math]::Round((($u.UT-$t0)/1000.0),2)
    $line=@((Get-Date).ToString("HH:mm:ss.fff"),$t,$u.M,$u.TS,$u.SG,$u.LP,$u.PH,
      $u.X,$u.Y,($u.H/10.0),$u.KX,$u.KY,($u.KH/10.0),$u.YA,(Wrap180 $u.YA),$u.GY,$u.PS,$u.EV,($u.KS/1000.0),
      $u.FC,$u.FS,$u.L,$u.C,$u.R,$u.CF) -join ","
    Add-Content -Path $Out -Value $line -Encoding ASCII
    $rows++
    Write-Host ("`rt={0,7:N1}s  seg={1} lap={2}  fused=({3},{4}) cm  heading={5,6:N1} deg  rows={6}   " -f $t,$u.SG,$u.LP,$u.X,$u.Y,($u.H/10.0),$rows) -NoNewline
   }
  }catch{ Write-Host "`n(no answer: $($_.Exception.Message))" -ForegroundColor Yellow }
  Start-Sleep -Milliseconds $PollMs
 }
}finally{ Write-Host "`nSaved $rows rows to $Out" }
