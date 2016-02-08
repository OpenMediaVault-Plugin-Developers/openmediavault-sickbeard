<?php

/**
 * Copyright (C) 2013-2015 OpenMediaVault Plugin Developers
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

require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/systemctl.inc";
require_once "openmediavault/module.inc";

class OMVModuleSickbeard extends OMVModuleServiceAbstract implements
    OMVINotifyListener,
    OMVIModuleServiceStatus
{
    private function getPluginName()
    {
        return strtolower($this->getName());
    }

    private function getEventMessagePath()
    {
        return sprintf("org.openmediavault.service.%s", $this->getPluginName());
    }

    private function getXpath()
    {
        return sprintf("/config/services/%s", $this->getPluginName());
    }

    public function getName()
    {
        return "Sickbeard";
    }

    public function getSickBeard1XPath()
    {
        return sprintf("%s/instances/instance1", $this->getXpath());
    }

    public function getSickBeard2XPath()
    {
        return sprintf("%s/instances/instance2", $this->getXpath());
    }

    public function getStatus()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getSickBeard1XPath());
        $newobject = $xmlConfig->get($this->getSickBeard2XPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        if (is_null($newobject)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $systemCtl = new OMVSystemCtl($this->getPluginName());

        return [
            "name"    => $this->getPluginName(),
            "title"   => gettext($this->getName()),
            "enabled" => boolval($object["enable"]),
            "running" => $systemCtl->isActive()
        ];
    }

    public function applyConfig()
    {
        $cmd = sprintf("omv-mkconf %s 2>&1", $this->getPluginName());
        if (0 !== $this->exec($cmd, $output)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode(PHP_EOL, $output)
            );
        }
    }

    public function startService()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getSickBeard1XPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        if (boolval($object["enable"])) {

            if ($object["branch"] == "Please choose a branch") {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a repository and branch"
                );
                return;
            }

            if ($object["repo"] == "Please choose a repository" ) {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a repository and branch"
                );
                return;
            }

            $systemCtl = new OMVSystemCtl($this->getPluginName());
            $systemCtl->enable(true);
        }

        if (boolval($newobject["enable"])) {

            if ($object["branch2"] == "Please choose a branch") {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a 2nd repository and branch"
                );
                return;
            }

            if ($object["repo2"] == "Please choose a repository" ) {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a 2nd repository and branch"
                );
                return;
            }

            //$systemCtl = new OMVSystemCtl("sickbeard2");
            //$systemCtl->enable(true);
        }
    }

    public function stopService()
    {
        $systemCtl = new OMVSystemCtl($this->getPluginName());
        $systemCtl->disable(true);
    }

    public function bindListeners(OMVNotifyDispatcher $dispatcher)
    {
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            [$this, "setDirty"]
        );
    }
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleSickbeard());
