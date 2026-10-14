<# plot_pose.ps1 - draw predicted vs fused path + headings from a log_pose.ps1 CSV and save a PNG.
   Needs Windows PowerShell 5.1 (the built-in "Windows PowerShell", NOT pwsh 7): uses .NET charting.
   powershell -ExecutionPolicy Bypass -File tools\plot_pose.ps1 -In run1.csv [-Out run1.png] [-RotateDeg 150]
   (If this fails use:  python tools\plot_pose.py run1.csv ) #>
param([Parameter(Mandatory=$true)][string]$In,[string]$Out="",[double]$RotateDeg=0)
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Windows.Forms.DataVisualization
if($Out -eq ""){ $Out=[IO.Path]::ChangeExtension($In,".png") }
$rows=Import-Csv $In
# reference track (same table as UNO_Assignment2/Track.h), 2 laps
$turn=@(0,-136.6,-103.4,30,90,120); $mm=@(600,1513,2200,433,1250,3200)
$ref=New-Object System.Collections.ArrayList; $x=0.0;$y=0.0;$h=0.0; [void]$ref.Add(@(0.0,0.0))
foreach($lap in 0,1){ $start= if($lap -eq 0){0}else{1}
 foreach($i in $start..5){ if($lap -eq 1 -and $i -eq 1){$h=0.0}
  $h+=$turn[$i]; $x+=$mm[$i]/10.0*[math]::Cos($h*[math]::PI/180); $y+=$mm[$i]/10.0*[math]::Sin($h*[math]::PI/180); [void]$ref.Add(@($x,$y)) } }
$c=[math]::Cos($RotateDeg*[math]::PI/180); $s=[math]::Sin($RotateDeg*[math]::PI/180)
function RX($px,$py){ $c*$px-$s*$py }  function RY($px,$py){ $s*$px+$c*$py }
$chart=New-Object System.Windows.Forms.DataVisualization.Charting.Chart; $chart.Width=1400; $chart.Height=650; $chart.BackColor="White"
$a1=New-Object System.Windows.Forms.DataVisualization.Charting.ChartArea "xy"; $a2=New-Object System.Windows.Forms.DataVisualization.Charting.ChartArea "hd"
$a1.Position=New-Object System.Windows.Forms.DataVisualization.Charting.ElementPosition(0,0,52,100); $a2.Position=New-Object System.Windows.Forms.DataVisualization.Charting.ElementPosition(52,0,48,100)
$a1.AxisX.Title="X (cm)"; $a1.AxisY.Title="Y (cm)"; $a2.AxisX.Title="time (s)"; $a2.AxisY.Title="heading (deg)"
$chart.ChartAreas.Add($a1); $chart.ChartAreas.Add($a2)
$leg=New-Object System.Windows.Forms.DataVisualization.Charting.Legend; $chart.Legends.Add($leg)
function NewSeries($name,$area,$color,$w,$dash){ $sr=New-Object System.Windows.Forms.DataVisualization.Charting.Series $name; $sr.ChartType="Line"; $sr.ChartArea=$area; $sr.Color=[Drawing.Color]::$color; $sr.BorderWidth=$w; if($dash){$sr.BorderDashStyle="Dash"}; $chart.Series.Add($sr); $sr }
$sRef=NewSeries "reference track" "xy" "Black" 1 $true; $sPred=NewSeries "predicted (model only)" "xy" "Red" 2 $false; $sFus=NewSeries "fused (model+gyro+events)" "xy" "Orange" 3 $false
foreach($p in $ref){ [void]$sRef.Points.AddXY((RX $p[0] $p[1]),(RY $p[0] $p[1])) }
$gH=NewSeries "gyro heading" "hd" "Orange" 2 $false; $pH=NewSeries "predicted heading" "hd" "Red" 2 $false; $fH=NewSeries "fused heading" "hd" "Green" 1 $true
foreach($r in $rows){
 [void]$sPred.Points.AddXY((RX ([double]$r.pred_x_cm) ([double]$r.pred_y_cm)),(RY ([double]$r.pred_x_cm) ([double]$r.pred_y_cm)))
 [void]$sFus.Points.AddXY((RX ([double]$r.fused_x_cm) ([double]$r.fused_y_cm)),(RY ([double]$r.fused_x_cm) ([double]$r.fused_y_cm)))
 [void]$gH.Points.AddXY([double]$r.t_s,[double]$r.gyro_heading_wrapped_deg); [void]$pH.Points.AddXY([double]$r.t_s,[double]$r.pred_heading_deg); [void]$fH.Points.AddXY([double]$r.t_s,[double]$r.fused_heading_deg)
}
$a1.AxisX.IsStartedFromZero=$false; $a1.AxisY.IsStartedFromZero=$false
$chart.SaveImage($Out,"Png"); Write-Host "saved $Out"
$last=$rows[-1]; Write-Host ("end fused ({0},{1}) cm  predicted ({2},{3}) cm   learned speed scale {4}" -f $last.fused_x_cm,$last.fused_y_cm,$last.pred_x_cm,$last.pred_y_cm,$last.speed_scale)
