<?php
require("default.php");

function getattachtmppath($userid,$utmpnum)
{
  $attachdir="cache/home/" . strtoupper(substr($userid,0,1)) . "/" . $userid . "/" . $utmpnum . "/upload"; 
  return $attachdir;
}
define("ANNOUNCENUMBER",5);
define("ARTICLESPERPAGE",20);
$SiteName="大运村BBS";

$HTMLTitle="大运村BBS";

$HTMLCharset="GB2312";

$DEFAULTStyle="defaultstyle";

$Banner="pic/ws.jpg";

$BannerURL="http://172.16.50.79";

//$SiteURL=$_SERVER['SERVER_NAME'];

$SiteURL="http://172.16.50.79";



define("ATTACHMAXSIZE","2097152");
define("ATTACHMAXCOUNT","3");
$section_nums = array("0", "1", "2", "3", "4");
$section_names = array(
    array("BBS 系统", "[系统]"),
    array("校园信息", "[信息]"),
    array("电脑技术", "[电脑]"),
    array("学术科学", "[学习]"),
    array("休闲娱乐", "[娱乐]")
);
$sectionCount=count($section_names);
?>
