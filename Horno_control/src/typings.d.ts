declare module '@arduino/arduino-iot-client' {
  export class ApiClient {
    static instance: any;
    authentications: {
      [key: string]: {
        accessToken: string;
      };
    };
  }

  export class DevicesV2Api {
    constructor(apiClient: ApiClient);
    devicesV2List(opts?: { xOrganization?: string }): Promise<any>;
  }
}
