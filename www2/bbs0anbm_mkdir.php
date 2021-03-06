<?php

require("bbs0anbm_pre.php");

if($has_perm_boards < 1)
	html_error_quit("您没有权限操作该目录。");

$text = "";
if(isset($_POST["title"]))
{
	if(BBS_ANN_AUTONAME == 0)
		$newfname = $_POST["filename"];
	else
		$newfname = "";
	$newtitle = $_POST["title"];
	$newbm = $_POST["bm"];
	$ret = bbs_ann_mkdir($filename, $newfname, $newtitle, $newbm);
	switch($ret)
	{
		case 0:
			header("Location: bbs0anbm.php?path=" . rawurlencode($path));
			exit;
		case -1:
			html_error_quit("精华区目录不存在。");
			break;
		case -2:
			$text = "错误：文件名包含非法字符。";
			break;
		case -3:
			$text = "错误：同名目录或文件已经存在。";
			break;
		case -4:
			html_error_quit("操作失败，可能有其他版主正在处理同一目录。");
			break;
	}
}
else
{	
	$newfname = "";
	$newtitle = "";
	$newbm = "";
}
	
page_header("创建目录", "精华区操作");

?>
<form action="bbs0anbm_mkdir.php?path=<?php echo rawurlencode($path); ?>" method="post" class="medium">
	<fieldset><legend>创建精华区目录</legend>
		<div class="inputs">
			<div style="color:#FF0000"><?php echo $text; ?></div>
<?php
if(BBS_ANN_AUTONAME == 0)
{
?>
			<label>文件名：</label><input type="text" maxlength="38" size="15" name="filename" value="<?php echo htmlspecialchars($newfname); ?>"><br>
<?php
}
?>
			<label>标　题：</label><input type="text" maxlength="38" size="38" name="title" value="<?php echo htmlspecialchars($newtitle); ?>"><br>
			<label>版　主：</label><input type="text" maxlength="38" size="15" name="bm" value="<?php echo htmlspecialchars($newbm); ?>"><br>
		</div>
	</fieldset>
	<div class="oper"><input type="submit" value="创建目录"> [<a href="bbs0anbm.php?path=<?php echo rawurlencode($path); ?>">返回精华区目录</a>]</div>
</form>
<?php

page_footer();
	
?>