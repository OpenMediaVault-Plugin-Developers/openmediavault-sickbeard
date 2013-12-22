<?php

/**
 * Copyright (C) 2013 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once "openmediavault/object.inc";
require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/rpcservice.inc";
require_once "openmediavault/notify.inc";

class OMVRpcServiceSickbeard extends OMVRpcServiceAbstract {

    public function getName() {
        return 'Sickbeard';
    }

    public function initialize() {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
    }

    public function getSettings($params, $context) {
        global $xmlConfig;
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));
        $object = $xmlConfig->get("//services/sickbeard");
        if(is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, "//services/sickbeard");
        }

        $object['enable']  = boolval($object['enable']);
        $object['showtab'] = boolval($object['showtab']);
        $object['sb_branch'] = intval($object['sb_branch']);
        if($object['sb_repo'] != "") {
          $object['sb_repo'] = intval($object['sb_repo']);
          return $object;
        }
        $object['sb_repo'] = '';
        return $object;
    }

    public function setSettings($params, $context) {
        global $xmlConfig;
        $oldObject = $xmlConfig->get("//services/couchpotato");
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));
        $this->validateMethodParams($params, '{
            "type" : "object",
            "properties" : {
                "enable"        : { "type" : "boolean" },
                "showtab"       : { "type" : "boolean" },
                "sb_branch"     : { "type" : "integer","enum":[0,1]},
                "sb_repo"       : { "type" : "integer","enum":[0,1]}
            }
        }');

        if($oldObject['sb_repo'] == "") {
            $oldObject['sb_repo'] = 3;
        }

        if($params['sb_repo'] == 1 && $params['sb_branch'] == 1)
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, "You CAN NOT use this branch with this repository.");

        if($params['sb_repo'] != $oldObject['sb_repo']) {
            //throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, $params['sb_repo'] . "-" . $oldObject['sb_repo']);
            if($params['sb_repo'] == 0) {
                $this->exec("/etc/init.d/sickbeard swap https://github.com/midgetspy/Sick-Beard.git master");
                $params['sb_branch'] = 0;
            }
            elseif($params['sb_repo'] == 1) {
                $this->exec("/etc/init.d/sickbeard swap https://github.com/xbianonpi/Sick-Beard-TPB.git ThePirateBay");
                $params['sb_branch'] = 0;
            }
        }

        if($params['sb_repo'] == $oldObject['sb_repo']) {
            if($params['sb_repo'] == 0) {
                if($params['sb_branch'] != $oldObject['sb_branch']) {
                    if($params['sb_branch'] == 0) {
                        $this->exec("/etc/init.d/sickbeard master");
                    }
                    if($params['sb_branch'] == 1) {
                        $this->exec("/etc/init.d/sickbeard develop");
                    }
                }
            }
        }

        $object = array(
            "enable"            => array_boolval($params, 'enable'),
            "showtab"           => array_boolval($params, 'showtab'),
            "sb_branch"         => $params['sb_branch'],
            "sb_repo"           => $params['sb_repo']
        );

        if(false === $xmlConfig->replace("//services/sickbeard", $object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, "//services/sickbeard");
        }

        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, "org.openmediavault.services.sickbeard", $object);
    }

}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceSickbeard());

?>
