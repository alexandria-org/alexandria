
<!DOCTYPE html>
	<!--[if IE 8]>
		<html xmlns="http://www.w3.org/1999/xhtml" class="ie8" lang="en-US">
	<![endif]-->
	<!--[if !(IE 8) ]><!-->
		<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US">
	<!--<![endif]-->
	<head>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
	<title>Log In &lsaquo; Automobiles Zone &#8212; WordPress</title>
	<link rel='dns-prefetch' href='//s.w.org' />
<script type='text/javascript' src='http://automobileszone.com/wp-includes/js/jquery/jquery.js?ver=1.12.4-wp'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-includes/js/jquery/jquery-migrate.min.js?ver=1.4.1'></script>
<link rel='stylesheet' id='genericons-css'  href='http://automobileszone.com/wp-content/plugins/jetpack/_inc/genericons/genericons/genericons.css?ver=3.1' type='text/css' media='all' />
<link rel='stylesheet' id='dashicons-css'  href='http://automobileszone.com/wp-includes/css/dashicons.min.css?ver=5.4.5' type='text/css' media='all' />
<link rel='stylesheet' id='buttons-css'  href='http://automobileszone.com/wp-includes/css/buttons.min.css?ver=5.4.5' type='text/css' media='all' />
<link rel='stylesheet' id='forms-css'  href='http://automobileszone.com/wp-admin/css/forms.min.css?ver=5.4.5' type='text/css' media='all' />
<link rel='stylesheet' id='l10n-css'  href='http://automobileszone.com/wp-admin/css/l10n.min.css?ver=5.4.5' type='text/css' media='all' />
<link rel='stylesheet' id='login-css'  href='http://automobileszone.com/wp-admin/css/login.min.css?ver=5.4.5' type='text/css' media='all' />
	<meta name='robots' content='noindex,noarchive' />
	<meta name='referrer' content='strict-origin-when-cross-origin' />
		<meta name="viewport" content="width=device-width" />
		</head>
	<body class="login no-js login-action-login wp-core-ui  locale-en-us">
	<script type="text/javascript">
		document.body.className = document.body.className.replace('no-js','js');
	</script>
		<div id="login">
		<h1><a href="https://wordpress.org/">Powered by WordPress</a></h1>
	
		<form name="loginform" id="loginform" action="http://automobileszone.com/wp-login.php" method="post">
			<p>
				<label for="user_login">Username or Email Address</label>
				<input type="text" name="log" id="user_login" class="input" value="" size="20" autocapitalize="off" />
			</p>

			<div class="user-pass-wrap">
				<label for="user_pass">Password</label>
				<div class="wp-pwd">
					<input type="password" name="pwd" id="user_pass" class="input password-input" value="" size="20" />
					<button type="button" class="button button-secondary wp-hide-pw hide-if-no-js" data-toggle="0" aria-label="Show password">
						<span class="dashicons dashicons-visibility" aria-hidden="true"></span>
					</button>
				</div>
			</div>
						<p class="forgetmenot"><input name="rememberme" type="checkbox" id="rememberme" value="forever"  /> <label for="rememberme">Remember Me</label></p>
			<p class="submit">
				<input type="submit" name="wp-submit" id="wp-submit" class="button button-primary button-large" value="Log In" />
									<input type="hidden" name="redirect_to" value="http://automobileszone.com/best-bronco-build-off-our-editors-weigh-in-on-their-ideal-suvs/" />
									<input type="hidden" name="testcookie" value="1" />
			</p>
		</form>

					<p id="nav">
									<a href="http://automobileszone.com/wp-login.php?action=lostpassword">Lost your password?</a>
								</p>
					<script type="text/javascript">
			function wp_attempt_focus() {setTimeout( function() {try {d = document.getElementById( "user_login" );d.focus(); d.select();} catch( er ) {}}, 200);}
wp_attempt_focus();
if ( typeof wpOnload === 'function' ) { wpOnload() }		</script>
				<p id="backtoblog"><a href="http://automobileszone.com/">
		&larr; Back to Automobiles Zone		</a></p>
			</div>
	<div class="jetpack-sso-wrap"><a href="http://automobileszone.com/wp-login.php?action=jetpack-sso&#038;redirect_to=http://automobileszone.com/best-bronco-build-off-our-editors-weigh-in-on-their-ideal-suvs/" class="jetpack-sso button">Log in with WordPress.com</a><style>
		.jetpack-sso.button {
			position: relative;
			padding-left: 37px;
		}
		.jetpack-sso.button:before {
			display: block;
			box-sizing: border-box;
			padding: 7px 0 0;
			text-align: center;
			position: absolute;
			top: -1px;
			left: -1px;
			border-radius: 2px 0 0 2px;
			content: '\f205';
			background: #0074a2;
			color: #fff;
			-webkit-font-smoothing: antialiased;
			width: 30px;
			height: 107%;
			height: calc( 100% + 2px );
			font: normal 22px/1 Genericons !important;
			text-shadow: none;
		}
		@media screen and (min-width: 783px) {
			.jetpack-sso.button:before {
				padding-top: 3px;
			}
		}
		.jetpack-sso.button:hover {
			border: 1px solid #aaa;
		}</style></div>		<style>
			#loginform {
				overflow: hidden;
				padding-bottom: 26px;
			}
			.jetpack-sso-wrap {
									float: right;
								margin: 1em 0 0;
				clear: right;
				display: block;
			}

					</style>
		<script>
			jQuery(document).ready(function($){
							$( '#loginform' ).append( $( '.jetpack-sso-wrap' ) );

				var $rememberme = $( '#rememberme' ),
					$ssoButton  = $( 'a.jetpack-sso.button' );

				$rememberme.on( 'change', function() {
					var url       = $ssoButton.prop( 'href' ),
						isChecked = $rememberme.prop( 'checked' ) ? 1 : 0;

					if ( url.match( /&rememberme=\d/ ) ) {
						url = url.replace( /&rememberme=\d/, '&rememberme=' + isChecked );
					} else {
						url += '&rememberme=' + isChecked;
					}

					$ssoButton.prop( 'href', url );
				} ).change();

			});
		</script>
		<link rel='stylesheet' id='jetpack_css-css'  href='http://automobileszone.com/wp-content/plugins/jetpack/css/jetpack.css?ver=3.9.7' type='text/css' media='all' />
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/jquery.js?ver=1.8.3'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/jquery-ui.js?ver=1.9.2'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/modernizr.js?ver=2.6.2'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/fitvids.js?ver=1.0.0'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/twitter.js?ver=1.0.0'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/user-rating.js?ver=1.0.0'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-content/themes/codilight/js/custom.js?ver=1.0.0'></script>
<script type='text/javascript'>
/* <![CDATA[ */
var _zxcvbnSettings = {"src":"http:\/\/automobileszone.com\/wp-includes\/js\/zxcvbn.min.js"};
/* ]]> */
</script>
<script type='text/javascript' src='http://automobileszone.com/wp-includes/js/zxcvbn-async.min.js?ver=1.0'></script>
<script type='text/javascript'>
/* <![CDATA[ */
var pwsL10n = {"unknown":"Password strength unknown","short":"Very weak","bad":"Weak","good":"Medium","strong":"Strong","mismatch":"Mismatch"};
/* ]]> */
</script>
<script type='text/javascript' src='http://automobileszone.com/wp-admin/js/password-strength-meter.min.js?ver=5.4.5'></script>
<script type='text/javascript' src='http://automobileszone.com/wp-includes/js/underscore.min.js?ver=1.8.3'></script>
<script type='text/javascript'>
/* <![CDATA[ */
var _wpUtilSettings = {"ajax":{"url":"\/wp-admin\/admin-ajax.php"}};
/* ]]> */
</script>
<script type='text/javascript' src='http://automobileszone.com/wp-includes/js/wp-util.min.js?ver=5.4.5'></script>
<script type='text/javascript'>
/* <![CDATA[ */
var userProfileL10n = {"warn":"Your new password has not been saved.","warnWeak":"Confirm use of weak password","show":"Show","hide":"Hide","cancel":"Cancel","ariaShow":"Show password","ariaHide":"Hide password"};
/* ]]> */
</script>
<script type='text/javascript' src='http://automobileszone.com/wp-admin/js/user-profile.min.js?ver=5.4.5'></script>
	<div class="clear"></div>
	</body>
	</html>
	
